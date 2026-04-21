#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "porchlight/core/types.hpp"

namespace porchlight {

struct QueueItem {
    std::int64_t rowid{0};
    std::string endpoint;
    std::string payload_json;
    int attempts{0};
};

class LocalEventStore {
public:
    explicit LocalEventStore(std::string path);
    ~LocalEventStore();
    LocalEventStore(const LocalEventStore&) = delete;
    LocalEventStore& operator=(const LocalEventStore&) = delete;

    void open();
    void insert_event(const EventRecord& event);
    void enqueue_outbound(const std::string& endpoint, const std::string& payload_json);
    std::vector<QueueItem> due_queue_items(int limit);
    void mark_delivered(std::int64_t rowid);
    void mark_failed(std::int64_t rowid, int attempts);
    std::int64_t queue_depth() const;
    std::int64_t event_count() const;

private:
    void exec(const std::string& sql) const;
    std::string path_;
    sqlite3* db_{nullptr};
};

}  // namespace porchlight
