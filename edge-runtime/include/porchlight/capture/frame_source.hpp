#pragma once

#include <fstream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "porchlight/config/config_manager.hpp"
#include "porchlight/core/types.hpp"
#include "porchlight/util/logger.hpp"

namespace porchlight {

class FrameSource {
public:
    virtual ~FrameSource() = default;
    virtual std::optional<Frame> next() = 0;
    virtual std::string name() const = 0;
};

class ReplaySource : public FrameSource {
public:
    explicit ReplaySource(std::string path);
    std::optional<Frame> next() override;
    std::string name() const override { return "replay"; }

private:
    std::ifstream input_;
    std::string path_;
    bool header_consumed_{false};
};

class MockSource : public FrameSource {
public:
    explicit MockSource(std::int64_t max_frames = 160);
    std::optional<Frame> next() override;
    std::string name() const override { return "mock"; }

private:
    std::int64_t frame_id_{0};
    std::int64_t max_frames_{160};
};

#ifdef PORCHLIGHT_WITH_OPENCV
std::unique_ptr<FrameSource> make_opencv_camera_source(const EdgeConfig& config, Logger& logger);
#endif

std::unique_ptr<FrameSource> make_frame_source(const EdgeConfig& config, Logger& logger);

}  // namespace porchlight
