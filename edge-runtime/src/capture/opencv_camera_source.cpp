#ifdef PORCHLIGHT_WITH_OPENCV
#include "porchlight/capture/frame_source.hpp"
#include <opencv2/opencv.hpp>

namespace porchlight {

class OpenCvCameraSource final : public FrameSource {
public:
    OpenCvCameraSource() : capture_(0) {}
    std::optional<Frame> next() override {
        cv::Mat frame_mat;
        if (!capture_.isOpened() || !capture_.read(frame_mat)) return std::nullopt;
        Frame frame;
        frame.id = frame_id_++;
        frame.timestamp_ms = frame.id * 100;
        frame.width = frame_mat.cols;
        frame.height = frame_mat.rows;
        frame.scenario = "live_camera";

        cv::Mat gray;
        cv::cvtColor(frame_mat, gray, cv::COLOR_BGR2GRAY);
        if (!previous_.empty()) {
            cv::Mat diff;
            cv::absdiff(gray, previous_, diff);
            frame.motion_score = static_cast<double>(cv::mean(diff)[0]) / 255.0;
        }
        previous_ = gray;
        return frame;
    }
    std::string name() const override { return "opencv-camera"; }
private:
    cv::VideoCapture capture_;
    cv::Mat previous_;
    std::int64_t frame_id_{0};
};

std::unique_ptr<FrameSource> make_opencv_camera_source(const EdgeConfig&, Logger&) {
    return std::make_unique<OpenCvCameraSource>();
}

}  // namespace porchlight
#endif
