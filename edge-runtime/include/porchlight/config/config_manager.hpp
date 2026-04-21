#pragma once

#include <string>

#include "porchlight/core/types.hpp"

namespace porchlight {

struct EdgeConfig {
    std::string device_id{"porch-pi-001"};
    std::string source_mode{"mock"};
    std::string replay_path{"demo-assets/scenarios/entry_sequence.frames.csv"};
    std::string backend_url{"http://127.0.0.1:8000"};
    std::string auth_token{"dev-device-token"};
    std::string store_path{"runtime-data/edge/porchlight_edge.db"};
    std::string snapshot_dir{"runtime-data/edge/snapshots"};
    bool publish_enabled{false};
    bool json_logs{false};
    std::int64_t max_frames{160};

    double motion_threshold{0.18};
    double person_threshold{0.72};
    double package_threshold{0.68};
    double vehicle_threshold{0.70};
    double linger_seconds{2.4};
    double dedupe_window_seconds{18};
    double cooldown_seconds{8};

    bool quiet_hours_enabled{false};
    std::string quiet_hours_start{"22:00"};
    std::string quiet_hours_end{"07:00"};
    std::string home_mode{"away"};
    Rect package_zone{0.52, 0.55, 0.33, 0.36};
};

class ConfigManager {
public:
    static EdgeConfig load(const std::string& path);
};

}  // namespace porchlight
