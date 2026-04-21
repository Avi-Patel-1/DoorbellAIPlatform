#pragma once

#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "porchlight/config/config_manager.hpp"
#include "porchlight/core/types.hpp"

namespace porchlight {

class MotionGate {
public:
    explicit MotionGate(double threshold);
    bool should_process(const Frame& frame) const;
private:
    double threshold_;
};

class EventAggregator {
public:
    explicit EventAggregator(EdgeConfig config);

    std::vector<EventRecord> evaluate(const Frame& frame, const std::vector<Detection>& detections, double processing_latency_ms);
    std::size_t surfaced_count() const { return surfaced_count_; }
    std::size_t suppressed_count() const { return suppressed_count_; }

private:
    EdgeConfig config_;
    Rect entry_zone_{0.16, 0.05, 0.72, 0.86};
    std::map<std::string, std::deque<std::pair<std::int64_t, double>>> confidence_history_;
    std::map<std::string, std::int64_t> last_emitted_ms_;
    std::map<std::string, std::int64_t> last_suppressed_ms_;

    bool person_present_{false};
    std::int64_t person_present_since_ms_{0};
    bool linger_emitted_for_current_visit_{false};

    bool package_present_{false};
    std::int64_t package_seen_since_ms_{0};
    std::int64_t package_absent_since_ms_{0};
    int package_stable_frames_{0};
    int repeated_motion_frames_{0};

    std::size_t surfaced_count_{0};
    std::size_t suppressed_count_{0};

    void update_history(std::int64_t timestamp_ms, const std::vector<Detection>& detections);
    std::vector<double> recent_confidences(const std::string& label, std::int64_t timestamp_ms, double window_ms = 2200.0) const;
    double stable_confidence(const std::string& label, std::int64_t timestamp_ms) const;
    bool can_emit(const std::string& type, std::int64_t timestamp_ms) const;
    bool should_sample_suppression(const std::string& type, std::int64_t timestamp_ms);
    EventRecord make_record(const Frame& frame, const std::string& type, double confidence, bool suppressed,
                            std::vector<std::string> reasons, std::string severity, double latency_ms,
                            const std::string& metadata_json = "{}") const;
    void mark(EventRecord& record);
    bool quiet_hours_active(std::int64_t timestamp_ms) const;
};

}  // namespace porchlight
