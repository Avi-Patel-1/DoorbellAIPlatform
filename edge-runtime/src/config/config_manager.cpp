#include "porchlight/config/config_manager.hpp"

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace porchlight {
namespace {

std::string trim(std::string value) {
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string strip_quotes(std::string value) {
    value = trim(value);
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

std::string expand_env(std::string value) {
    const auto start = value.find("${");
    if (start == std::string::npos) return value;
    const auto end = value.find('}', start + 2);
    if (end == std::string::npos) return value;
    const auto key = value.substr(start + 2, end - start - 2);
    const char* env = std::getenv(key.c_str());
    return value.substr(0, start) + (env ? env : "") + value.substr(end + 1);
}

bool parse_bool(const std::map<std::string, std::string>& kv, const std::string& key, bool fallback) {
    const auto it = kv.find(key);
    if (it == kv.end()) return fallback;
    const std::string v = it->second;
    return v == "true" || v == "1" || v == "yes" || v == "on";
}

double parse_double(const std::map<std::string, std::string>& kv, const std::string& key, double fallback) {
    const auto it = kv.find(key);
    if (it == kv.end() || it->second.empty()) return fallback;
    return std::stod(it->second);
}

std::int64_t parse_i64(const std::map<std::string, std::string>& kv, const std::string& key, std::int64_t fallback) {
    const auto it = kv.find(key);
    if (it == kv.end() || it->second.empty()) return fallback;
    return std::stoll(it->second);
}

std::string parse_str(const std::map<std::string, std::string>& kv, const std::string& key, const std::string& fallback) {
    const auto it = kv.find(key);
    if (it == kv.end()) return fallback;
    return it->second;
}

}  // namespace

EdgeConfig ConfigManager::load(const std::string& path) {
    EdgeConfig cfg;
    std::ifstream in(path);
    if (!in) {
        return cfg;
    }

    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);
        if (trim(line).empty()) continue;
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const auto key = trim(line.substr(0, colon));
        auto value = strip_quotes(line.substr(colon + 1));
        kv[key] = expand_env(value);
    }

    cfg.device_id = parse_str(kv, "device_id", cfg.device_id);
    cfg.source_mode = parse_str(kv, "source_mode", cfg.source_mode);
    cfg.replay_path = parse_str(kv, "replay_path", cfg.replay_path);
    cfg.backend_url = parse_str(kv, "backend_url", cfg.backend_url);
    cfg.auth_token = parse_str(kv, "auth_token", cfg.auth_token);
    cfg.store_path = parse_str(kv, "store_path", cfg.store_path);
    cfg.snapshot_dir = parse_str(kv, "snapshot_dir", cfg.snapshot_dir);
    cfg.publish_enabled = parse_bool(kv, "publish_enabled", cfg.publish_enabled);
    cfg.json_logs = parse_bool(kv, "json_logs", cfg.json_logs);
    cfg.max_frames = parse_i64(kv, "max_frames", cfg.max_frames);
    cfg.motion_threshold = parse_double(kv, "motion_threshold", cfg.motion_threshold);
    cfg.person_threshold = parse_double(kv, "person_threshold", cfg.person_threshold);
    cfg.package_threshold = parse_double(kv, "package_threshold", cfg.package_threshold);
    cfg.vehicle_threshold = parse_double(kv, "vehicle_threshold", cfg.vehicle_threshold);
    cfg.linger_seconds = parse_double(kv, "linger_seconds", cfg.linger_seconds);
    cfg.dedupe_window_seconds = parse_double(kv, "dedupe_window_seconds", cfg.dedupe_window_seconds);
    cfg.cooldown_seconds = parse_double(kv, "cooldown_seconds", cfg.cooldown_seconds);
    cfg.quiet_hours_enabled = parse_bool(kv, "quiet_hours_enabled", cfg.quiet_hours_enabled);
    cfg.quiet_hours_start = parse_str(kv, "quiet_hours_start", cfg.quiet_hours_start);
    cfg.quiet_hours_end = parse_str(kv, "quiet_hours_end", cfg.quiet_hours_end);
    cfg.home_mode = parse_str(kv, "home_mode", cfg.home_mode);
    cfg.package_zone.x = parse_double(kv, "package_zone_x", cfg.package_zone.x);
    cfg.package_zone.y = parse_double(kv, "package_zone_y", cfg.package_zone.y);
    cfg.package_zone.w = parse_double(kv, "package_zone_w", cfg.package_zone.w);
    cfg.package_zone.h = parse_double(kv, "package_zone_h", cfg.package_zone.h);

    return cfg;
}

}  // namespace porchlight
