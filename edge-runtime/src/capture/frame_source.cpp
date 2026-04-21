#include "porchlight/capture/frame_source.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace porchlight {
namespace {

std::vector<std::string> split(const std::string& value, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

Detection parse_detection(const std::string& token) {
    const auto parts = split(token, '@');
    Detection det;
    if (parts.size() != 6) return det;
    det.label = parts[0];
    det.confidence = std::stod(parts[1]);
    det.box.x = std::stod(parts[2]);
    det.box.y = std::stod(parts[3]);
    det.box.w = std::stod(parts[4]);
    det.box.h = std::stod(parts[5]);
    return det;
}

std::vector<std::string> parse_csv_row(const std::string& line) {
    std::vector<std::string> columns;
    std::string current;
    bool in_quote = false;
    for (char c : line) {
        if (c == '"') {
            in_quote = !in_quote;
        } else if (c == ',' && !in_quote) {
            columns.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    columns.push_back(current);
    return columns;
}

}  // namespace

ReplaySource::ReplaySource(std::string path) : path_(std::move(path)) {
    input_.open(path_);
}

std::optional<Frame> ReplaySource::next() {
    if (!input_) return std::nullopt;
    std::string line;
    if (!header_consumed_) {
        std::getline(input_, line);
        header_consumed_ = true;
    }
    while (std::getline(input_, line)) {
        if (line.empty()) continue;
        const auto cols = parse_csv_row(line);
        if (cols.size() < 5) continue;
        Frame frame;
        frame.id = std::stoll(cols[0]);
        frame.timestamp_ms = std::stoll(cols[1]);
        frame.motion_score = std::stod(cols[2]);
        frame.scenario = cols[3];
        if (!cols[4].empty()) {
            for (const auto& token : split(cols[4], ';')) {
                if (!token.empty()) frame.detections.push_back(parse_detection(token));
            }
        }
        return frame;
    }
    return std::nullopt;
}

MockSource::MockSource(std::int64_t max_frames) : max_frames_(max_frames > 0 ? max_frames : 160) {}

std::optional<Frame> MockSource::next() {
    if (frame_id_ >= max_frames_) return std::nullopt;
    Frame frame;
    frame.id = frame_id_;
    frame.timestamp_ms = frame_id_ * 100;
    frame.scenario = "mock";

    auto add_person = [&](double conf, double x, double y) {
        frame.detections.push_back(Detection{"person", conf, Rect{x, y, 0.24, 0.56}});
    };
    auto add_package = [&](double conf) {
        frame.detections.push_back(Detection{"package", conf, Rect{0.60, 0.65, 0.20, 0.16}});
    };

    if (frame_id_ < 16) {
        frame.motion_score = 0.04;
        frame.scenario = "quiet_porch";
    } else if (frame_id_ < 42) {
        frame.motion_score = 0.34;
        frame.scenario = "person_approaches";
        add_person(0.66 + 0.012 * (frame_id_ - 16), 0.35, 0.22);
    } else if (frame_id_ < 78) {
        frame.motion_score = 0.38;
        frame.scenario = "person_lingers";
        add_person(0.90, 0.36, 0.12);
    } else if (frame_id_ < 92) {
        frame.motion_score = 0.24;
        frame.scenario = "low_confidence_motion";
        if (frame_id_ % 2 == 0) add_person(0.51, 0.69, 0.30);
    } else if (frame_id_ < 120) {
        frame.motion_score = 0.32;
        frame.scenario = "package_delivered";
        add_package(0.70 + std::min(0.20, 0.01 * (frame_id_ - 92)));
        if (frame_id_ < 100) add_person(0.82, 0.38, 0.18);
    } else if (frame_id_ < 138) {
        frame.motion_score = 0.09;
        frame.scenario = "package_waiting";
        add_package(0.84);
    } else if (frame_id_ < 150) {
        frame.motion_score = 0.29;
        frame.scenario = "package_removed_transition";
        add_person(0.82, 0.42, 0.16);
    } else {
        frame.motion_score = 0.06;
        frame.scenario = "package_removed";
    }

    ++frame_id_;
    return frame;
}

std::unique_ptr<FrameSource> make_frame_source(const EdgeConfig& config, Logger& logger) {
    if (config.source_mode == "replay") {
        logger.info("using replay source", {{"path", config.replay_path}});
        return std::make_unique<ReplaySource>(config.replay_path);
    }
    if (config.source_mode == "live") {
#ifdef PORCHLIGHT_WITH_OPENCV
        logger.info("OpenCV live source selected");
        return make_opencv_camera_source(config, logger);
#else
        logger.warn("live source requested but OpenCV support is not enabled; falling back to mock source");
        return std::make_unique<MockSource>(config.max_frames);
#endif
    }
    logger.info("using mock source");
    return std::make_unique<MockSource>(config.max_frames);
}

}  // namespace porchlight
