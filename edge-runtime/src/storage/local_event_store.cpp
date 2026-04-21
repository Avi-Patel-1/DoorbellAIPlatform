#include "porchlight/storage/local_event_store.hpp"

#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace porchlight {

LocalEventStore::LocalEventStore(std::string path) : path_(std::move(path)) {}

LocalEventStore::~LocalEventStore() {
    if (db_) sqlite3_close(db_);
}

void LocalEventStore::open() {
    const auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("failed to open sqlite database: " + path_);
    }
    exec("PRAGMA journal_mode=WAL;");
    exec("CREATE TABLE IF NOT EXISTS events ("
         "id TEXT PRIMARY KEY, timestamp_ms INTEGER NOT NULL, device_id TEXT NOT NULL, type TEXT NOT NULL, "
         "suppressed INTEGER NOT NULL, confidence REAL NOT NULL, payload_json TEXT NOT NULL);");
    exec("CREATE TABLE IF NOT EXISTS outbound_queue ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT, endpoint TEXT NOT NULL, payload_json TEXT NOT NULL, attempts INTEGER NOT NULL DEFAULT 0, "
         "available_at_ms INTEGER NOT NULL, created_at_ms INTEGER NOT NULL);");
}

void LocalEventStore::exec(const std::string& sql) const {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown sqlite error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}

void LocalEventStore::insert_event(const EventRecord& event) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO events(id,timestamp_ms,device_id,type,suppressed,confidence,payload_json) VALUES(?,?,?,?,?,?,?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) throw std::runtime_error("prepare insert_event failed");
    const auto payload = event.to_json();
    sqlite3_bind_text(stmt, 1, event.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, event.timestamp_ms);
    sqlite3_bind_text(stmt, 3, event.device_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, event.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, event.suppressed ? 1 : 0);
    sqlite3_bind_double(stmt, 6, event.confidence);
    sqlite3_bind_text(stmt, 7, payload.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("insert_event failed");
    }
    sqlite3_finalize(stmt);
}

void LocalEventStore::enqueue_outbound(const std::string& endpoint, const std::string& payload_json) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO outbound_queue(endpoint,payload_json,attempts,available_at_ms,created_at_ms) VALUES(?,?,0,?,?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) throw std::runtime_error("prepare enqueue failed");
    const auto now = now_ms();
    sqlite3_bind_text(stmt, 1, endpoint.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, payload_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_int64(stmt, 4, now);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error("enqueue_outbound failed");
    }
    sqlite3_finalize(stmt);
}

std::vector<QueueItem> LocalEventStore::due_queue_items(int limit) {
    std::vector<QueueItem> items;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id,endpoint,payload_json,attempts FROM outbound_queue WHERE available_at_ms <= ? ORDER BY id LIMIT ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) throw std::runtime_error("prepare due_queue failed");
    sqlite3_bind_int64(stmt, 1, now_ms());
    sqlite3_bind_int(stmt, 2, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QueueItem item;
        item.rowid = sqlite3_column_int64(stmt, 0);
        item.endpoint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        item.payload_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        item.attempts = sqlite3_column_int(stmt, 3);
        items.push_back(std::move(item));
    }
    sqlite3_finalize(stmt);
    return items;
}

void LocalEventStore::mark_delivered(std::int64_t rowid) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "DELETE FROM outbound_queue WHERE id=?", -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int64(stmt, 1, rowid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void LocalEventStore::mark_failed(std::int64_t rowid, int attempts) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "UPDATE outbound_queue SET attempts=?, available_at_ms=? WHERE id=?", -1, &stmt, nullptr) != SQLITE_OK) return;
    const auto next_attempts = attempts + 1;
    const auto backoff_ms = static_cast<std::int64_t>(std::min(60000.0, std::pow(2.0, next_attempts) * 1000.0));
    sqlite3_bind_int(stmt, 1, next_attempts);
    sqlite3_bind_int64(stmt, 2, now_ms() + backoff_ms);
    sqlite3_bind_int64(stmt, 3, rowid);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::int64_t LocalEventStore::queue_depth() const {
    sqlite3_stmt* stmt = nullptr;
    std::int64_t value = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM outbound_queue", -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

std::int64_t LocalEventStore::event_count() const {
    sqlite3_stmt* stmt = nullptr;
    std::int64_t value = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM events", -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

}  // namespace porchlight
