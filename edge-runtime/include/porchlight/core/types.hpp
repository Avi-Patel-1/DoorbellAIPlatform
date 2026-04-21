#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "porchlight/util/json.hpp"

namespace porchlight {

inline std::int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

struct Rect {
    double x{0};
    double y{0};
    double w{0};
    double h{0};

    double center_x() const { return x + w / 2.0; }
    double center_y() const { return y + h / 2.0; }
    double area() const { return w * h; }

    bool contains_point(double px, double py) const {
        return px >= x && py >= y && px <= x + w && py <= y + h;
    }

    bool contains_center(const Rect& other) const {
        return contains_point(other.center_x(), other.center_y());
    }

    bool intersects(const Rect& other) const {
        return !(other.x > x + w || other.x + other.w < x || other.y > y + h || other.y + other.h < y);
    }

    std::string to_json() const {
        return "{\"x\":" + json::number(x) + ",\"y\":" + json::number(y) +
               ",\"w\":" + json::number(w) + ",\"h\":" + json::number(h) + "}";
    }
};

struct Detection {
    std::string label;
    double confidence{0};
    Rect box;

    std::string to_json() const {
        return "{\"label\":" + json::quote(label) + ",\"confidence\":" + json::number(confidence) +
               ",\"box\":" + box.to_json() + "}";
    }
};

struct Frame {
    std::int64_t id{0};
    std::int64_t timestamp_ms{0};
    double motion_score{0};
    int width{1280};
    int height{720};
    std::string scenario;
    std::vector<Detection> detections;
};

struct EventRecord {
    std::string id;
    std::string device_id;
    std::string type;
    std::string severity{"info"};
    std::int64_t timestamp_ms{0};
    double confidence{0};
    bool suppressed{false};
    std::vector<std::string> reasons;
    std::vector<double> confidence_history;
    std::string snapshot_ref;
    std::string clip_ref;
    double processing_latency_ms{0};
    std::string metadata_json{"{}"};

    std::string explanation() const {
        std::ostringstream out;
        for (std::size_t i = 0; i < reasons.size(); ++i) {
            if (i) out << "; ";
            out << reasons[i];
        }
        return out.str();
    }

    std::string to_json() const {
        std::ostringstream out;
        out << "{"
            << "\"id\":" << json::quote(id) << ","
            << "\"device_id\":" << json::quote(device_id) << ","
            << "\"type\":" << json::quote(type) << ","
            << "\"severity\":" << json::quote(severity) << ","
            << "\"timestamp_ms\":" << timestamp_ms << ","
            << "\"confidence\":" << json::number(confidence) << ","
            << "\"suppressed\":" << json::bool_value(suppressed) << ","
            << "\"reasons\":" << json::string_array(reasons) << ","
            << "\"explanation\":" << json::quote(explanation()) << ","
            << "\"confidence_history\":" << json::number_array(confidence_history) << ","
            << "\"snapshot_ref\":" << json::quote(snapshot_ref) << ","
            << "\"clip_ref\":" << json::quote(clip_ref) << ","
            << "\"processing_latency_ms\":" << json::number(processing_latency_ms) << ","
            << "\"metadata\":" << (metadata_json.empty() ? "{}" : metadata_json)
            << "}";
        return out.str();
    }
};

inline std::string make_event_id(const std::string& device_id, const std::string& type, std::int64_t timestamp_ms, std::int64_t frame_id) {
    std::ostringstream out;
    out << device_id << "-" << type << "-" << timestamp_ms << "-" << frame_id;
    return out.str();
}

}  // namespace porchlight
