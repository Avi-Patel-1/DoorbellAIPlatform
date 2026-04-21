#include "porchlight/logic/event_aggregator.hpp"

#include <cassert>
#include <iostream>

using namespace porchlight;

int main() {
    EdgeConfig config;
    config.device_id = "test-device";
    config.person_threshold = 0.70;
    config.package_threshold = 0.68;
    config.dedupe_window_seconds = 20;
    config.linger_seconds = 0.5;
    EventAggregator aggregator(config);

    int emitted_person = 0;
    int duplicate_suppressed = 0;
    for (int i = 0; i < 8; ++i) {
        Frame frame;
        frame.id = i;
        frame.timestamp_ms = i * 100;
        frame.motion_score = 0.35;
        frame.scenario = "unit-person";
        std::vector<Detection> dets{{"person", 0.82, Rect{0.35, 0.12, 0.24, 0.56}}};
        auto events = aggregator.evaluate(frame, dets, 0.2);
        for (const auto& ev : events) {
            if (ev.type == "person_at_door" && !ev.suppressed) emitted_person++;
            if (ev.type == "duplicate_event_suppressed") duplicate_suppressed++;
        }
    }
    assert(emitted_person == 1);
    assert(aggregator.surfaced_count() >= 1);

    EventAggregator packages(config);
    int delivered = 0;
    for (int i = 0; i < 7; ++i) {
        Frame frame;
        frame.id = i;
        frame.timestamp_ms = i * 100;
        frame.motion_score = 0.28;
        frame.scenario = "unit-package";
        std::vector<Detection> dets{{"package", 0.82, Rect{0.60, 0.65, 0.20, 0.16}}};
        auto events = packages.evaluate(frame, dets, 0.2);
        for (const auto& ev : events) if (ev.type == "package_delivered" && !ev.suppressed) delivered++;
    }
    assert(delivered == 1);

    std::cout << "event logic tests passed\n";
    return 0;
}
