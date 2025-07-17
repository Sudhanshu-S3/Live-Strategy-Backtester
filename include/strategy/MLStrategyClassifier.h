#ifndef ML_STRATEGY_CLASSIFIER_H
#define ML_STRATEGY_CLASSIFIER_H

#include "../data/DataTypes.h"
#include "Strategy.h"
#include <string>
#include <vector>
#include <memory>

// Forward declaration
class OnnxRuntime; // Example ML runtime

class MLStrategyClassifier {
public:
    MLStrategyClassifier(const std::string& model_path);
    virtual ~MLStrategyClassifier() = default;

    // Classifies the market and returns a list of recommended strategy names
    virtual std::vector<std::string> classify(const MarketState& state);

    // A more advanced classification using recent market data
    virtual std::vector<std::string> classify(const std::vector<Bar>& recent_data);

private:
    // This would be a pointer to the actual ML model runner
    // For example, using ONNX runtime
    // std::unique_ptr<OnnxRuntime> model_session_; 
    
    std::string model_path_;

    void load_model();
};

#endif // ML_STRATEGY_CLASSIFIER_H 