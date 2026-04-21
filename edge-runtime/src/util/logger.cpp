#include "porchlight/util/logger.hpp"
#include "porchlight/util/json.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace porchlight {

namespace {
std::string iso_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
}  // namespace

Logger::Logger(bool json_logs, LogLevel level) : json_logs_(json_logs), min_level_(level) {}

void Logger::set_json(bool enabled) { json_logs_ = enabled; }
void Logger::set_level(LogLevel level) { min_level_ = level; }

void Logger::debug(const std::string& message, const std::map<std::string, std::string>& fields) { log(LogLevel::Debug, message, fields); }
void Logger::info(const std::string& message, const std::map<std::string, std::string>& fields) { log(LogLevel::Info, message, fields); }
void Logger::warn(const std::string& message, const std::map<std::string, std::string>& fields) { log(LogLevel::Warn, message, fields); }
void Logger::error(const std::string& message, const std::map<std::string, std::string>& fields) { log(LogLevel::Error, message, fields); }

const char* Logger::name(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warn: return "warn";
        case LogLevel::Error: return "error";
    }
    return "info";
}

void Logger::log(LogLevel level, const std::string& message, const std::map<std::string, std::string>& fields) {
    if (static_cast<int>(level) < static_cast<int>(min_level_)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (json_logs_) {
        std::cerr << "{\"ts\":" << json::quote(iso_timestamp())
                  << ",\"level\":" << json::quote(name(level))
                  << ",\"message\":" << json::quote(message);
        for (const auto& kv : fields) {
            std::cerr << "," << json::quote(kv.first) << ":" << json::quote(kv.second);
        }
        std::cerr << "}" << std::endl;
    } else {
        std::cerr << "[" << iso_timestamp() << "] " << name(level) << " " << message;
        for (const auto& kv : fields) {
            std::cerr << " " << kv.first << "=" << kv.second;
        }
        std::cerr << std::endl;
    }
}

}  // namespace porchlight
