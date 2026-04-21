#pragma once

#include <memory>
#include <string>
#include <vector>

#include "porchlight/config/config_manager.hpp"
#include "porchlight/core/types.hpp"
#include "porchlight/util/logger.hpp"

namespace porchlight {

class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;
    virtual std::vector<Detection> detect(const Frame& frame) = 0;
    virtual std::string name() const = 0;
};

class MockInferenceEngine : public InferenceEngine {
public:
    std::vector<Detection> detect(const Frame& frame) override;
    std::string name() const override { return "mock-inference"; }
};

class ModelFallbackInferenceEngine : public InferenceEngine {
public:
    explicit ModelFallbackInferenceEngine(Logger& logger);
    std::vector<Detection> detect(const Frame& frame) override;
    std::string name() const override { return "model-fallback"; }
private:
    Logger& logger_;
    bool warned_{false};
};

std::unique_ptr<InferenceEngine> make_inference_engine(const EdgeConfig& config, Logger& logger);

}  // namespace porchlight
