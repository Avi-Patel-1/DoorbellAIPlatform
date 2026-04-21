#include "porchlight/config/config_manager.hpp"
#include "porchlight/runtime/device_runtime.hpp"

#include <iostream>
#include <string>

using porchlight::ConfigManager;
using porchlight::DeviceRuntime;
using porchlight::RuntimeOptions;

namespace {
void usage() {
    std::cout << "porchlight_edge --config <path> [--mode mock|replay|live] [--replay <path>] [--max-frames N] "
              << "[--no-publish] [--output-json path] [--benchmark-json path] [--simulate-offline-frames N]\n";
}
}  // namespace

int main(int argc, char** argv) {
    std::string config_path = "config/edge.demo.yaml";
    std::string mode_override;
    std::string replay_override;
    long long max_frames_override = -1;
    RuntimeOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto need_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << name << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "--help" || arg == "-h") { usage(); return 0; }
        if (arg == "--config") config_path = need_value(arg);
        else if (arg == "--mode") mode_override = need_value(arg);
        else if (arg == "--replay") replay_override = need_value(arg);
        else if (arg == "--max-frames") max_frames_override = std::stoll(need_value(arg));
        else if (arg == "--output-json") options.output_json_path = need_value(arg);
        else if (arg == "--benchmark-json") options.benchmark_json_path = need_value(arg);
        else if (arg == "--simulate-offline-frames") options.simulate_offline_frames = std::stoi(need_value(arg));
        else if (arg == "--no-publish") { options.publish_override_set = true; options.publish_enabled = false; }
        else if (arg == "--publish") { options.publish_override_set = true; options.publish_enabled = true; }
        else {
            std::cerr << "unknown argument: " << arg << "\n";
            usage();
            return 2;
        }
    }

    auto config = ConfigManager::load(config_path);
    if (!mode_override.empty()) config.source_mode = mode_override;
    if (!replay_override.empty()) config.replay_path = replay_override;
    if (max_frames_override >= 0) config.max_frames = max_frames_override;
    DeviceRuntime runtime(config, options);
    return runtime.run();
}
