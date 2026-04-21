#include "porchlight/inference/inference_engine.hpp"

namespace porchlight {

std::vector<Detection> MockInferenceEngine::detect(const Frame& frame) {
    return frame.detections;
}

ModelFallbackInferenceEngine::ModelFallbackInferenceEngine(Logger& logger) : logger_(logger) {}

std::vector<Detection> ModelFallbackInferenceEngine::detect(const Frame& frame) {
    if (!warned_) {
        logger_.warn("model backend unavailable; using replay/mock detections embedded in frame source");
        warned_ = true;
    }
    return frame.detections;
}

std::unique_ptr<InferenceEngine> make_inference_engine(const EdgeConfig&, Logger& logger) {
    // The default path keeps the demo CPU-only and dependency-light. A production build can
    // replace this with an ONNX Runtime implementation behind the same interface.
    return std::make_unique<ModelFallbackInferenceEngine>(logger);
}

}  // namespace porchlight
