#include "porchlight/health/health_collector.hpp"
#include "porchlight/core/types.hpp"
#include "porchlight/util/json.hpp"

#include <fstream>
#include <sstream>
#include <unistd.h>

namespace porchlight {
namespace {

double loadavg_1m() {
    std::ifstream in("/proc/loadavg");
    double value = 0.0;
    if (in) in >> value;
    return value;
}

double rss_mb() {
    std::ifstream in("/proc/self/statm");
    long pages = 0;
    long resident = 0;
    if (in) in >> pages >> resident;
    const long page_size = sysconf(_SC_PAGESIZE);
    return static_cast<double>(resident * page_size) / (1024.0 * 1024.0);
}

}  // namespace

std::string HealthSnapshot::to_json() const {
    std::ostringstream out;
    out << "{"
        << "\"device_id\":" << json::quote(device_id) << ","
        << "\"timestamp_ms\":" << timestamp_ms << ","
        << "\"fps\":" << json::number(fps) << ","
        << "\"cpu_load_1m\":" << json::number(cpu_load_1m) << ","
        << "\"memory_rss_mb\":" << json::number(memory_rss_mb) << ","
        << "\"queue_depth\":" << queue_depth << ","
        << "\"dropped_frames\":" << dropped_frames << ","
        << "\"network_status\":" << json::quote(network_status) << ","
        << "\"uptime_seconds\":" << json::number(uptime_seconds)
        << "}";
    return out.str();
}

HealthCollector::HealthCollector(std::string device_id) : device_id_(std::move(device_id)), start_ms_(now_ms()) {}

HealthSnapshot HealthCollector::collect(double fps, std::int64_t queue_depth, std::int64_t dropped_frames, std::string network_status) const {
    HealthSnapshot snapshot;
    snapshot.device_id = device_id_;
    snapshot.timestamp_ms = now_ms();
    snapshot.fps = fps;
    snapshot.cpu_load_1m = loadavg_1m();
    snapshot.memory_rss_mb = rss_mb();
    snapshot.queue_depth = queue_depth;
    snapshot.dropped_frames = dropped_frames;
    snapshot.network_status = std::move(network_status);
    snapshot.uptime_seconds = static_cast<double>(snapshot.timestamp_ms - start_ms_) / 1000.0;
    return snapshot;
}

}  // namespace porchlight
