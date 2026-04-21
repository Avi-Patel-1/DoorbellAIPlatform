#pragma once

#include <fstream>
#include <string>

#include "porchlight/comms/http_client.hpp"
#include "porchlight/config/config_manager.hpp"
#include "porchlight/health/health_collector.hpp"
#include "porchlight/logic/event_aggregator.hpp"
#include "porchlight/storage/local_event_store.hpp"
#include "porchlight/util/logger.hpp"

namespace porchlight {

struct RuntimeOptions {
    std::string output_json_path;
    std::string benchmark_json_path;
    bool publish_override_set{false};
    bool publish_enabled{false};
    int simulate_offline_frames{0};
};

class DeviceRuntime {
public:
    DeviceRuntime(EdgeConfig config, RuntimeOptions options);
    int run();

private:
    void flush_queue(LocalEventStore& store, HttpClient& client, bool online);
    EdgeConfig config_;
    RuntimeOptions options_;
    Logger logger_;
};

}  // namespace porchlight
