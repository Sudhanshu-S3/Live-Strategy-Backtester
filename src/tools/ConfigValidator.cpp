#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

// Forward declarations
bool validateConfig(const nlohmann::json& config);
bool validateStrategy(const nlohmann::json& strategy);
void printValidationError(const std::string& field, const std::string& issue);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ConfigValidator <config_file.json>\n";
        std::cout << "Validates strategy configuration files for the Live Strategy Backtester\n";
        return 1;
    }
    
    try {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << argv[1] << "\n";
            return 1;
        }
        
        nlohmann::json config;
        try {
            config = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << "\n";
            return 1;
        }
        
        if (validateConfig(config)) {
            std::cout << "Configuration is valid and can be used for backtesting.\n";
            return 0;
        }
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

bool validateConfig(const nlohmann::json& config) {
    std::vector<std::pair<std::string, bool>> requiredFields = {
        {"run_mode", config.contains("run_mode") && config["run_mode"].is_string()},
        {"symbols", config.contains("symbols") && config["symbols"].is_array() && !config["symbols"].empty()},
        {"initial_capital", config.contains("initial_capital") && config["initial_capital"].is_number()},
        {"strategies", config.contains("strategies") && config["strategies"].is_array() && !config["strategies"].empty()},
    };

    bool isValid = true;
    for (const auto& [field, valid] : requiredFields) {
        if (!valid) {
            printValidationError(field, "missing or invalid format");
            isValid = false;
        }
    }
    
    // Check data section
    if (config.contains("data") && config["data"].is_object()) {
        auto data = config["data"];
        std::vector<std::pair<std::string, bool>> dataFields = {
            {"start_date", data.contains("start_date") && data["start_date"].is_string()},
            {"end_date", data.contains("end_date") && data["end_date"].is_string()},
        };
        
        for (const auto& [field, valid] : dataFields) {
            if (!valid) {
                printValidationError("data." + field, "missing or invalid format");
                isValid = false;
            }
        }
    } else {
        printValidationError("data", "missing or invalid format");
        isValid = false;
    }
    
    // Check each strategy
    if (config.contains("strategies") && config["strategies"].is_array()) {
        for (size_t i = 0; i < config["strategies"].size(); ++i) {
            if (!validateStrategy(config["strategies"][i])) {
                std::cout << "Strategy at index " << i << " is invalid\n";
                isValid = false;
            }
        }
    }
    
    return isValid;
}

bool validateStrategy(const nlohmann::json& strategy) {
    bool isValid = true;
    
    std::vector<std::pair<std::string, bool>> requiredFields = {
        {"name", strategy.contains("name") && strategy["name"].is_string()},
        {"active", strategy.contains("active") && strategy["active"].is_boolean()},
        {"symbol", strategy.contains("symbol") && strategy["symbol"].is_string()},
        {"params", strategy.contains("params") && strategy["params"].is_object()}
    };

    for (const auto& [field, valid] : requiredFields) {
        if (!valid) {
            printValidationError("strategy." + field, "missing or invalid format");
            isValid = false;
        }
    }
    
    // Strategy-specific validation
    if (strategy.contains("name") && strategy["name"].is_string()) {
        std::string name = strategy["name"];
        
        if (name == "ORDER_BOOK_IMBALANCE") {
            if (strategy.contains("params") && strategy["params"].is_object()) {
                auto params = strategy["params"];
                std::vector<std::pair<std::string, bool>> paramFields = {
                    {"lookback_levels", params.contains("lookback_levels") && params["lookback_levels"].is_number()},
                    {"imbalance_threshold", params.contains("imbalance_threshold") && params["imbalance_threshold"].is_number()}
                };
                
                for (const auto& [field, valid] : paramFields) {
                    if (!valid) {
                        printValidationError("strategy.params." + field, "missing or invalid format");
                        isValid = false;
                    }
                }
            }
        } else if (name == "SIMPLE_MOVING_AVERAGE_CROSSOVER") {
            if (strategy.contains("params") && strategy["params"].is_object()) {
                auto params = strategy["params"];
                std::vector<std::pair<std::string, bool>> paramFields = {
                    {"short_window", params.contains("short_window") && params["short_window"].is_number()},
                    {"long_window", params.contains("long_window") && params["long_window"].is_number()}
                };
                
                for (const auto& [field, valid] : paramFields) {
                    if (!valid) {
                        printValidationError("strategy.params." + field, "missing or invalid format");
                        isValid = false;
                    }
                }
            }
        }
        // Add more strategy-specific validation here
    }
    
    return isValid;
}

void printValidationError(const std::string& field, const std::string& issue) {
    std::cerr << "Validation error: Field '" << field << "' " << issue << "\n";
}