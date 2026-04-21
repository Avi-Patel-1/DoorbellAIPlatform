#include "porchlight/runtime/device_runtime.hpp"

#include "porchlight/capture/frame_source.hpp"
#include "porchlight/inference/inference_engine.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

namespace porchlight {

DeviceRuntime::DeviceRuntime(EdgeConfig config, RuntimeOptions options)
    : config_(std::move(config)), options_(std::move(options)), logger_(config_.json_logs) {
    if (options_.publish_override_set) config_.publish_enabled = options_.publish_enabled;
}

void DeviceRuntime::flush_queue(LocalEventStore& store, HttpClient& client, bool online) {
    if (!config_.publish_enabled || !online) return;
    auto items = store.due_queue_items(12);
    for (const auto& item : items) {
        const bool ok = client.post_json(item.endpoint, item.payload_json);
        if (ok) {
            store.mark_delivered(item.rowid);
            logger_.debug("flushed queued message", {{"endpoint", item.endpoint}, {"rowid", std::to_string(item.rowid)}});
        } else {
            store.mark_failed(item.rowid, item.attempts);
            logger_.warn("backend publish failed; message retained", {{"endpoint", item.endpoint}, {"attempts", std::to_string(item.attempts + 1)}});
            break;
        }
    }
}

int DeviceRuntime::run() {
    logger_.info("starting edge runtime", {{"device_id", config_.device_id}, {"mode", config_.source_mode}});
    std::filesystem::create_directories(config_.snapshot_dir);
    LocalEventStore store(config_.store_path);
    store.open();

    auto source = make_frame_source(config_, logger_);
    auto inference = make_inference_engine(config_, logger_);
    MotionGate gate(config_.motion_threshold);
    EventAggregator aggregator(config_);
    HealthCollector health(config_.device_id);
    HttpClient client(config_.backend_url, config_.auth_token);

    std::ofstream event_out;
    if (!options_.output_json_path.empty()) {
        std::filesystem::create_directories(std::filesystem::path(options_.output_json_path).parent_path());
        event_out.open(options_.output_json_path);
    }

    std::vector<double> latencies;
    std::int64_t frames = 0;
    std::int64_t processed = 0;
    std::int64_t dropped = 0;
    std::int64_t offline_queue_max = 0;
    std::string network = config_.publish_enabled ? "online" : "disabled";
    const auto start = std::chrono::steady_clock::now();

    while (true) {
        if (config_.max_frames > 0 && frames >= config_.max_frames) break;
        auto frame_opt = source->next();
        if (!frame_opt) break;
        auto frame = *frame_opt;
        ++frames;

        const bool online = !(options_.simulate_offline_frames > 0 && frames <= options_.simulate_offline_frames);
        network = online ? (config_.publish_enabled ? "online" : "disabled") : "offline-simulated";

        const auto before = std::chrono::steady_clock::now();
        if (!gate.should_process(frame)) {
            ++dropped;
            continue;
        }
        const auto detections = inference->detect(frame);
        const auto after_infer = std::chrono::steady_clock::now();
        const double latency_ms = std::chrono::duration<double, std::milli>(after_infer - before).count();
        auto events = aggregator.evaluate(frame, detections, latency_ms);
        ++processed;
        latencies.push_back(latency_ms);

        for (const auto& event : events) {
            store.insert_event(event);
            const auto payload = event.to_json();
            if (event_out) event_out << payload << "\n";
            if (config_.publish_enabled) store.enqueue_outbound("/events", payload);
            logger_.info(event.suppressed ? "event suppressed" : "event emitted",
                         {{"event_id", event.id}, {"type", event.type}, {"confidence", json::number(event.confidence)},
                          {"queue_depth", std::to_string(store.queue_depth())}, {"reason", event.explanation()}});
        }

        if (frames % 15 == 0) {
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            const double fps = elapsed > 0 ? static_cast<double>(frames) / elapsed : 0.0;
            const auto snapshot = health.collect(fps, store.queue_depth(), dropped, network);
            if (config_.publish_enabled) store.enqueue_outbound("/devices/" + config_.device_id + "/metrics", snapshot.to_json());
            logger_.debug("health snapshot", {{"fps", json::number(snapshot.fps)}, {"queue_depth", std::to_string(snapshot.queue_depth)}, {"network", network}});
        }

        flush_queue(store, client, online);
        offline_queue_max = std::max<std::int64_t>(offline_queue_max, store.queue_depth());
    }

    // Final best-effort flush once connectivity is restored.
    flush_queue(store, client, true);

    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const double fps = elapsed > 0 ? static_cast<double>(frames) / elapsed : 0.0;
    double avg_latency = 0.0;
    double p95_latency = 0.0;
    if (!latencies.empty()) {
        avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / static_cast<double>(latencies.size());
        std::sort(latencies.begin(), latencies.end());
        const std::size_t idx = std::min(latencies.size() - 1, static_cast<std::size_t>(latencies.size() * 0.95));
        p95_latency = latencies[idx];
    }

    if (!options_.benchmark_json_path.empty()) {
        std::filesystem::create_directories(std::filesystem::path(options_.benchmark_json_path).parent_path());
        std::ofstream out(options_.benchmark_json_path);
        out << "{"
            << "\"device_id\":" << json::quote(config_.device_id) << ","
            << "\"frames\":" << frames << ","
            << "\"processed_frames\":" << processed << ","
            << "\"dropped_frames\":" << dropped << ","
            << "\"fps\":" << json::number(fps, 4) << ","
            << "\"avg_processing_latency_ms\":" << json::number(avg_latency, 5) << ","
            << "\"p95_processing_latency_ms\":" << json::number(p95_latency, 5) << ","
            << "\"meaningful_alerts\":" << aggregator.surfaced_count() << ","
            << "\"suppressed_alerts\":" << aggregator.suppressed_count() << ","
            << "\"queue_depth_final\":" << store.queue_depth() << ","
            << "\"queue_depth_peak\":" << offline_queue_max
            << "}";
    }

    logger_.info("edge runtime stopped", {{"frames", std::to_string(frames)}, {"fps", json::number(fps)},
        {"meaningful_alerts", std::to_string(aggregator.surfaced_count())}, {"suppressed_alerts", std::to_string(aggregator.suppressed_count())},
        {"queue_depth", std::to_string(store.queue_depth())}});
    return 0;
}

}  // namespace porchlight
