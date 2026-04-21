#include "porchlight/logic/event_aggregator.hpp"

#include <algorithm>
#include <ctime>
#include <numeric>
#include <sstream>

namespace porchlight {

MotionGate::MotionGate(double threshold) : threshold_(threshold) {}

bool MotionGate::should_process(const Frame& frame) const {
    return frame.motion_score >= threshold_ || !frame.detections.empty();
}

EventAggregator::EventAggregator(EdgeConfig config) : config_(std::move(config)) {}

void EventAggregator::update_history(std::int64_t timestamp_ms, const std::vector<Detection>& detections) {
    for (const auto& det : detections) {
        auto& q = confidence_history_[det.label];
        q.emplace_back(timestamp_ms, det.confidence);
        while (!q.empty() && timestamp_ms - q.front().first > 5000) q.pop_front();
    }
}

std::vector<double> EventAggregator::recent_confidences(const std::string& label, std::int64_t timestamp_ms, double window_ms) const {
    std::vector<double> values;
    const auto it = confidence_history_.find(label);
    if (it == confidence_history_.end()) return values;
    for (const auto& item : it->second) {
        if (timestamp_ms - item.first <= window_ms) values.push_back(item.second);
    }
    return values;
}

double EventAggregator::stable_confidence(const std::string& label, std::int64_t timestamp_ms) const {
    const auto values = recent_confidences(label, timestamp_ms);
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

bool EventAggregator::can_emit(const std::string& type, std::int64_t timestamp_ms) const {
    const auto it = last_emitted_ms_.find(type);
    if (it == last_emitted_ms_.end()) return true;
    return (timestamp_ms - it->second) >= static_cast<std::int64_t>(config_.dedupe_window_seconds * 1000.0);
}

bool EventAggregator::should_sample_suppression(const std::string& type, std::int64_t timestamp_ms) {
    const auto it = last_suppressed_ms_.find(type);
    if (it != last_suppressed_ms_.end() && timestamp_ms - it->second < 4500) {
        return false;
    }
    last_suppressed_ms_[type] = timestamp_ms;
    return true;
}

bool EventAggregator::quiet_hours_active(std::int64_t timestamp_ms) const {
    if (!config_.quiet_hours_enabled) return false;
    const std::time_t seconds = timestamp_ms / 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &seconds);
#else
    localtime_r(&seconds, &tm);
#endif
    auto parse_minutes = [](const std::string& hhmm) {
        if (hhmm.size() < 5) return 0;
        return std::stoi(hhmm.substr(0, 2)) * 60 + std::stoi(hhmm.substr(3, 2));
    };
    const int current = tm.tm_hour * 60 + tm.tm_min;
    const int start = parse_minutes(config_.quiet_hours_start);
    const int end = parse_minutes(config_.quiet_hours_end);
    if (start <= end) return current >= start && current < end;
    return current >= start || current < end;
}

EventRecord EventAggregator::make_record(const Frame& frame, const std::string& type, double confidence, bool suppressed,
                                          std::vector<std::string> reasons, std::string severity, double latency_ms,
                                          const std::string& metadata_json) const {
    EventRecord event;
    event.id = make_event_id(config_.device_id, type, frame.timestamp_ms, frame.id);
    event.device_id = config_.device_id;
    event.type = type;
    event.severity = std::move(severity);
    event.timestamp_ms = frame.timestamp_ms;
    event.confidence = confidence;
    event.suppressed = suppressed;
    event.reasons = std::move(reasons);
    event.confidence_history = recent_confidences(type.find("package") != std::string::npos ? "package" : "person", frame.timestamp_ms);
    event.snapshot_ref = "snapshots/" + event.id + ".jpg";
    event.clip_ref = "clips/" + event.id + ".mp4";
    event.processing_latency_ms = latency_ms;
    std::ostringstream metadata;
    if (metadata_json == "{}") {
        metadata << "{\"frame_id\":" << frame.id << ",\"scenario\":" << json::quote(frame.scenario)
                 << ",\"motion_score\":" << json::number(frame.motion_score)
                 << ",\"package_zone\":" << config_.package_zone.to_json() << "}";
        event.metadata_json = metadata.str();
    } else {
        event.metadata_json = metadata_json;
    }
    return event;
}

void EventAggregator::mark(EventRecord& record) {
    if (record.suppressed) {
        ++suppressed_count_;
    } else {
        ++surfaced_count_;
        last_emitted_ms_[record.type] = record.timestamp_ms;
    }
}

std::vector<EventRecord> EventAggregator::evaluate(const Frame& frame, const std::vector<Detection>& detections, double processing_latency_ms) {
    std::vector<EventRecord> results;
    update_history(frame.timestamp_ms, detections);

    const bool disarmed = config_.home_mode == "disarmed";
    const bool quiet = quiet_hours_active(frame.timestamp_ms);

    double best_person = 0.0;
    double best_package = 0.0;
    double best_vehicle = 0.0;
    std::optional<Detection> best_person_det;
    std::optional<Detection> best_package_det;

    for (const auto& det : detections) {
        if (det.label == "person" && det.box.intersects(entry_zone_)) {
            if (det.confidence > best_person) { best_person = det.confidence; best_person_det = det; }
        } else if (det.label == "package" && config_.package_zone.contains_center(det.box)) {
            if (det.confidence > best_package) { best_package = det.confidence; best_package_det = det; }
        } else if (det.label == "vehicle") {
            best_vehicle = std::max(best_vehicle, det.confidence);
        }
    }

    if (best_person >= config_.person_threshold) {
        if (!person_present_) {
            person_present_ = true;
            person_present_since_ms_ = frame.timestamp_ms;
            linger_emitted_for_current_visit_ = false;
        }
        repeated_motion_frames_ = 0;
        const auto stable = stable_confidence("person", frame.timestamp_ms);
        const bool stable_enough = recent_confidences("person", frame.timestamp_ms).size() >= 3 && stable >= config_.person_threshold;
        if (stable_enough) {
            if (disarmed) {
                if (should_sample_suppression("disarmed_person_suppressed", frame.timestamp_ms)) {
                    auto ev = make_record(frame, "person_at_door", stable, true,
                        {"Person detected but device is disarmed by current home/away policy"}, "info", processing_latency_ms);
                    mark(ev); results.push_back(std::move(ev));
                }
            } else if (quiet) {
                if (should_sample_suppression("quiet_hours_person_suppressed", frame.timestamp_ms)) {
                    auto ev = make_record(frame, "person_at_door", stable, true,
                        {"Person detected during quiet hours; non-package alert suppressed"}, "info", processing_latency_ms);
                    mark(ev); results.push_back(std::move(ev));
                }
            } else if (can_emit("person_at_door", frame.timestamp_ms)) {
                auto ev = make_record(frame, "person_at_door", stable, false,
                    {"Person detected in entry zone with stable confidence above threshold",
                     "Temporal smoothing confirmed detection across recent frames"}, "medium", processing_latency_ms);
                mark(ev); results.push_back(std::move(ev));
            } else if (should_sample_suppression("duplicate_person_suppressed", frame.timestamp_ms)) {
                auto ev = make_record(frame, "duplicate_event_suppressed", stable, true,
                    {"Duplicate person event suppressed because a similar alert was emitted inside the dedupe window"}, "info", processing_latency_ms);
                mark(ev); results.push_back(std::move(ev));
            }
        }

        const double linger_ms = config_.linger_seconds * 1000.0;
        if (!linger_emitted_for_current_visit_ && frame.timestamp_ms - person_present_since_ms_ >= linger_ms && can_emit("visitor_linger", frame.timestamp_ms)) {
            auto ev = make_record(frame, "visitor_linger", std::max(best_person, stable), false,
                {"Person remained in entry zone beyond linger threshold",
                 "Linger state persisted across multiple frames rather than a single motion spike"}, "high", processing_latency_ms);
            mark(ev); results.push_back(std::move(ev));
            linger_emitted_for_current_visit_ = true;
        }
    } else {
        if (person_present_ && best_person < config_.person_threshold * 0.6) {
            person_present_ = false;
            linger_emitted_for_current_visit_ = false;
        }
        if (best_person > 0.0 && best_person < config_.person_threshold && should_sample_suppression("low_confidence_person_suppressed", frame.timestamp_ms)) {
            auto ev = make_record(frame, "low_confidence_motion_suppressed", best_person, true,
                {"Person-like detection did not meet confidence threshold",
                 "Alert withheld until confidence is stable across frames"}, "info", processing_latency_ms);
            mark(ev); results.push_back(std::move(ev));
        }
    }

    if (best_package >= config_.package_threshold) {
        package_absent_since_ms_ = 0;
        ++package_stable_frames_;
        if (!package_present_) {
            if (package_seen_since_ms_ == 0) package_seen_since_ms_ = frame.timestamp_ms;
            const bool persisted = package_stable_frames_ >= 5 || (frame.timestamp_ms - package_seen_since_ms_) >= 1200;
            if (persisted && can_emit("package_delivered", frame.timestamp_ms)) {
                package_present_ = true;
                auto ev = make_record(frame, "package_delivered", std::max(best_package, stable_confidence("package", frame.timestamp_ms)), false,
                    {"Package-sized object entered the configured package zone",
                     "Object persisted across consecutive frames before alerting"}, "high", processing_latency_ms);
                mark(ev); results.push_back(std::move(ev));
            }
        } else if (should_sample_suppression("duplicate_package_suppressed", frame.timestamp_ms) && !can_emit("package_delivered", frame.timestamp_ms)) {
            auto ev = make_record(frame, "duplicate_event_suppressed", best_package, true,
                {"Package remains in zone; duplicate delivery alert suppressed"}, "info", processing_latency_ms);
            mark(ev); results.push_back(std::move(ev));
        }
    } else {
        package_stable_frames_ = 0;
        package_seen_since_ms_ = 0;
        if (package_present_) {
            if (package_absent_since_ms_ == 0) package_absent_since_ms_ = frame.timestamp_ms;
            const bool absent_long_enough = frame.timestamp_ms - package_absent_since_ms_ >= 1600;
            if (absent_long_enough && can_emit("package_removed", frame.timestamp_ms)) {
                package_present_ = false;
                auto ev = make_record(frame, "package_removed", 0.88, false,
                    {"Previously tracked package disappeared from package zone",
                     "Removal confirmed after absence persisted for multiple frames"}, "high", processing_latency_ms);
                mark(ev); results.push_back(std::move(ev));
            }
        }
    }

    if (frame.motion_score >= config_.motion_threshold && best_person < config_.person_threshold && best_package < config_.package_threshold) {
        ++repeated_motion_frames_;
        if (repeated_motion_frames_ >= 8 && can_emit("repeated_motion", frame.timestamp_ms)) {
            auto ev = make_record(frame, "repeated_motion", frame.motion_score, false,
                {"Repeated motion observed without a qualifying object detection",
                 "Surfaced as a low-severity operational signal instead of a visitor alert"}, "low", processing_latency_ms);
            mark(ev); results.push_back(std::move(ev));
            repeated_motion_frames_ = 0;
        } else if (should_sample_suppression("motion_without_detection_suppressed", frame.timestamp_ms)) {
            auto ev = make_record(frame, "motion_without_detection_suppressed", frame.motion_score, true,
                {"Motion detected but no qualifying person or package detection was stable enough to alert"}, "info", processing_latency_ms);
            mark(ev); results.push_back(std::move(ev));
        }
    } else if (frame.motion_score < config_.motion_threshold * 0.7) {
        repeated_motion_frames_ = 0;
    }

    if (best_vehicle >= config_.vehicle_threshold && should_sample_suppression("vehicle_suppressed", frame.timestamp_ms)) {
        auto ev = make_record(frame, "irrelevant_vehicle_suppressed", best_vehicle, true,
            {"Vehicle detected outside entry context; porch alert suppressed"}, "info", processing_latency_ms);
        mark(ev); results.push_back(std::move(ev));
    }

    return results;
}

}  // namespace porchlight
