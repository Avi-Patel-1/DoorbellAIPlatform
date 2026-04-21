#pragma once

#include <cstdint>
#include <string>

#include "porchlight/config/config_manager.hpp"

namespace porchlight {

struct HealthSnapshot {
    std::string device_id;
    std::int64_t timestamp_ms{0};
    double fps{0};
    double cpu_load_1m{0};
    double memory_rss_mb{0};
    std::int64_t queue_depth{0};
    std::int64_t dropped_frames{0};
    std::string network_status{"unknown"};
    double uptime_seconds{0};

    std::string to_json() const;
};

class HealthCollector {
public:
    explicit HealthCollector(std::string device_id);
    HealthSnapshot collect(double fps, std::int64_t queue_depth, std::int64_t dropped_frames, std::string network_status) const;
private:
    std::string device_id_;
    std::int64_t start_ms_;
};

}  // namespace porchlight
