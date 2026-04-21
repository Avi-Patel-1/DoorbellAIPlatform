#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

namespace porchlight {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    explicit Logger(bool json_logs = false, LogLevel level = LogLevel::Info);

    void set_json(bool enabled);
    void set_level(LogLevel level);
    void debug(const std::string& message, const std::map<std::string, std::string>& fields = {});
    void info(const std::string& message, const std::map<std::string, std::string>& fields = {});
    void warn(const std::string& message, const std::map<std::string, std::string>& fields = {});
    void error(const std::string& message, const std::map<std::string, std::string>& fields = {});

private:
    void log(LogLevel level, const std::string& message, const std::map<std::string, std::string>& fields);
    static const char* name(LogLevel level);
    bool json_logs_;
    LogLevel min_level_;
    std::mutex mutex_;
};

}  // namespace porchlight
