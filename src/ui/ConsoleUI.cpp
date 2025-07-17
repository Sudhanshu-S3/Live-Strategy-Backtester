#include "../../include/ui/ConsoleUI.h"
#include "../../include/core/Backtester.h"
#include "../../include/core/Optimizer.h"
#include "../../include/core/WalkForwardAnalyzer.h"
#include "../../include/core/MonteCarloSimulator.h"
#include "../../include/analytics/Analytics.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <iomanip>

ConsoleUI::ConsoleUI() {
    loadConfig();
}

void ConsoleUI::loadConfig() {
    try {
        std::ifstream config_file("config.json");
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }
        config_ = nlohmann::json::parse(config_file);
    } catch (const std::exception& e) {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        std::cerr << "Using default configuration." << std::endl;
        // Create a minimal default configuration
        config_ = {
            {"run_mode", "BACKTEST"},
            {"symbols", {"BTCUSDT"}},
            {"initial_capital", 100000.0},
            {"data", {
                {"start_date", "2025-07-13"},
                {"end_date", "2025-07-14"},
                {"trade_data_dir", "data"},
                {"book_data_dir", "data"},
                {"historical_data_fallback_dir", "historical_data"}
            }},
            {"data_handler", {
                {"live_host", "stream.binance.com"},
                {"live_port", "9443"},
                {"live_target", "/ws/btcusdt@trade"}
            }},
            {"risk", {
                {"risk_per_trade_pct", 0.01}
            }},
            {"strategies", {
                {{"name", "ORDER_BOOK_IMBALANCE"}, {"active", true}, {"symbol", "BTCUSDT"}, 
                 {"params", {{"lookback_levels", 10}, {"imbalance_threshold", 1.5}}}}
            }}
        };
    }
}

void ConsoleUI::saveConfig() {
    try {
        std::ofstream config_file("config.json");
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json for writing");
        }
        config_file << std::setw(4) << config_ << std::endl;
        std::cout << "Configuration saved successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration: " << e.what() << std::endl;
    }
}

void ConsoleUI::displayMainMenu() {
    while (true) {
        std::cout << "\n====== Live Strategy Backtester ======\n";
        std::cout << "1. Run Backtest (Historical Data)\n";
        std::cout << "2. Run Live Shadow Trading\n";
        std::cout << "3. Compare Live vs. Backtest Performance\n";
        std::cout << "4. Strategy Optimization\n";
        std::cout << "5. Walk-Forward Analysis\n";
        std::cout << "6. Monte Carlo Simulation\n";
        std::cout << "7. Configure Settings\n";
        std::cout << "8. Exit\n";
        std::cout << "Select an option: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline from input buffer

        switch (choice) {
            case 1:
                runBacktest();
                break;
            case 2:
                runLiveShadowTrading();
                break;
            case 3:
                compareLiveVsBacktest();
                break;
            case 4:
                runOptimization();
                break;
            case 5:
                runWalkForwardAnalysis();
                break;
            case 6:
                runMonteCarloSimulation();
                break;
            case 7:
                configureSettings();
                break;
            case 8:
                std::cout << "Exiting application. Goodbye!\n";
                return;
            default:
                std::cout << "Invalid option. Please try again.\n";
                break;
        }
    }
}

void ConsoleUI::runBacktest() {
    std::cout << "\n====== Historical Data Backtesting ======\n";
    
    nlohmann::json backtest_config = config_;
    backtest_config["run_mode"] = "BACKTEST";
    
    try {
        std::cout << "Starting backtest with configuration:\n";
        std::cout << "- Capital: " << backtest_config["initial_capital"] << "\n";
        std::cout << "- Symbols: ";
        for (const auto& symbol : backtest_config["symbols"]) {
            std::cout << symbol << " ";
        }
        std::cout << "\n";
        std::cout << "- Date Range: " << backtest_config["data"]["start_date"] << " to " 
                  << backtest_config["data"]["end_date"] << "\n";
        std::cout << "- Strategy: " << backtest_config["strategies"][0]["name"] << "\n";
        
        Backtester backtester(backtest_config);
        backtester.run();
        
        auto portfolio = backtester.getPortfolio();
        if (portfolio) {
            std::cout << "\nBacktest completed successfully!\n";
            last_backtest_portfolio_ = portfolio;
            // Print some key metrics
            std::cout << "Final Equity: " << portfolio->get_total_equity() << "\n";
            std::cout << "Total Return: " << (portfolio->get_total_equity() / portfolio->getInitialCapital() - 1.0) * 100.0 << "%\n";
            // You can add more metrics here
        }
    } catch (const std::exception& e) {
        std::cerr << "Backtest encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::runLiveShadowTrading() {
    std::cout << "\n====== Live Shadow Trading ======\n";
    
    nlohmann::json live_config = config_;
    live_config["run_mode"] = "SHADOW";
    
    try {
        std::cout << "Starting live shadow trading with configuration:\n";
        std::cout << "- Capital: " << live_config["initial_capital"] << "\n";
        std::cout << "- Symbols: ";
        for (const auto& symbol : live_config["symbols"]) {
            std::cout << symbol << " ";
        }
        std::cout << "\n";
        std::cout << "- WebSocket Host: " << live_config["data_handler"]["live_host"] << "\n";
        std::cout << "- Strategy: " << live_config["strategies"][0]["name"] << "\n";
        
        std::cout << "\nConnecting to live data feed...\n";
        Backtester backtester(live_config);
        
        std::cout << "Press Enter to stop live trading...";
        std::cin.get();
        
        std::cout << "Stopping live trading...\n";
        backtester.run();
        
        auto portfolio = backtester.getPortfolio();
        if (portfolio) {
            std::cout << "\nLive trading session completed!\n";
            last_live_portfolio_ = portfolio;
            // Print some key metrics
            std::cout << "Final Equity: " << portfolio->get_total_equity() << "\n";
            std::cout << "Total Return: " << (portfolio->get_total_equity() / portfolio->getInitialCapital() - 1.0) * 100.0 << "%\n";
            // You can add more metrics here
        }
    } catch (const std::exception& e) {
        std::cerr << "Live trading encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::compareLiveVsBacktest() {
    std::cout << "\n====== Live vs. Backtest Comparison ======\n";
    
    if (!last_live_portfolio_ || !last_backtest_portfolio_) {
        std::cout << "You need to run both a backtest and a live session before comparing them.\n";
        return;
    }
    
    try {
        Analytics analytics(config_.contains("analytics") ? config_["analytics"] : nlohmann::json({}));
        analytics.comparePerformance(last_live_portfolio_, last_backtest_portfolio_);
    } catch (const std::exception& e) {
        std::cerr << "Comparison encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::runOptimization() {
    std::cout << "\n====== Strategy Optimization ======\n";
    
    nlohmann::json opt_config = config_;
    opt_config["run_mode"] = "OPTIMIZATION";
    
    // Ensure optimization section exists
    if (!opt_config.contains("optimization") || !opt_config["optimization"].is_object()) {
        opt_config["optimization"] = {
            {"enabled", true},
            {"strategy_to_optimize", opt_config["strategies"][0]["name"]},
            {"param_ranges", {
                {"lookback_levels_start", 5},
                {"lookback_levels_end", 20},
                {"lookback_levels_step", 5},
                {"imbalance_threshold_start", 1.0},
                {"imbalance_threshold_end", 2.0},
                {"imbalance_threshold_step", 0.5}
            }}
        };
    }
    
    try {
        std::cout << "Starting optimization with configuration:\n";
        std::cout << "- Strategy to optimize: " << opt_config["optimization"]["strategy_to_optimize"] << "\n";
        std::cout << "- Parameter ranges: " << opt_config["optimization"]["param_ranges"] << "\n";
        
        Optimizer optimizer(opt_config);
        auto best_params = optimizer.run();
        
        if (!best_params.empty()) {
            std::cout << "\nOptimization completed successfully!\n";
            std::cout << "Best parameters: " << best_params << "\n";
            std::cout << "Best metric: " << optimizer.getBestMetric() << "\n";
            
            // Ask if user wants to update the config with the best parameters
            std::cout << "Do you want to update your configuration with these parameters? (y/n): ";
            char choice;
            std::cin >> choice;
            std::cin.ignore(); // Clear newline
            
            if (choice == 'y' || choice == 'Y') {
                // Update the strategy parameters in the config
                for (auto& strategy : config_["strategies"]) {
                    if (strategy["name"] == opt_config["optimization"]["strategy_to_optimize"]) {
                        strategy["params"] = best_params;
                        break;
                    }
                }
                saveConfig();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Optimization encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::runWalkForwardAnalysis() {
    std::cout << "\n====== Walk-Forward Analysis ======\n";
    
    nlohmann::json wf_config = config_;
    wf_config["run_mode"] = "WALK_FORWARD";
    
    // Ensure walk_forward section exists
    if (!wf_config.contains("walk_forward") || !wf_config["walk_forward"].is_object()) {
        wf_config["walk_forward"] = {
            {"in_sample_months", 3},
            {"out_of_sample_months", 1},
            {"start_date", "2025-01-01"},
            {"end_date", "2025-07-14"},
            {"strategy_to_test", wf_config["strategies"][0]["name"]}
        };
    }
    
    try {
        std::cout << "Starting walk-forward analysis with configuration:\n";
        std::cout << "- In-sample period: " << wf_config["walk_forward"]["in_sample_months"] << " months\n";
        std::cout << "- Out-of-sample period: " << wf_config["walk_forward"]["out_of_sample_months"] << " months\n";
        std::cout << "- Date range: " << wf_config["walk_forward"]["start_date"] << " to " 
                  << wf_config["walk_forward"]["end_date"] << "\n";
        std::cout << "- Strategy to test: " << wf_config["walk_forward"]["strategy_to_test"] << "\n";
        
        WalkForwardAnalyzer wf_analyzer(wf_config);
        wf_analyzer.run();
        
        std::cout << "\nWalk-forward analysis completed!\n";
    } catch (const std::exception& e) {
        std::cerr << "Walk-forward analysis encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::runMonteCarloSimulation() {
    std::cout << "\n====== Monte Carlo Simulation ======\n";
    
    nlohmann::json mc_config = config_;
    mc_config["run_mode"] = "MONTE_CARLO";
    
    // Ensure monte_carlo section exists
    if (!mc_config.contains("monte_carlo") || !mc_config["monte_carlo"].is_object()) {
        mc_config["monte_carlo"] = {
            {"num_simulations", 1000},
            {"confidence_level", 0.95}
        };
    }
    
    try {
        std::cout << "Starting Monte Carlo simulation with configuration:\n";
        std::cout << "- Number of simulations: " << mc_config["monte_carlo"]["num_simulations"] << "\n";
        std::cout << "- Confidence level: " << mc_config["monte_carlo"]["confidence_level"] << "\n";
        
        MonteCarloSimulator mc_simulator(mc_config);
        mc_simulator.run(mc_config["monte_carlo"]["num_simulations"]);
        
        std::cout << "\nMonte Carlo simulation completed!\n";
    } catch (const std::exception& e) {
        std::cerr << "Monte Carlo simulation encountered an error: " << e.what() << std::endl;
    }
}

void ConsoleUI::configureSettings() {
    while (true) {
        std::cout << "\n====== Configuration Menu ======\n";
        std::cout << "1. Configure Strategy\n";
        std::cout << "2. Configure Data Sources\n";
        std::cout << "3. Configure Risk Parameters\n";
        std::cout << "4. Configure Analysis Settings\n";
        std::cout << "5. View Current Configuration\n";
        std::cout << "6. Return to Main Menu\n";
        std::cout << "Select an option: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline

        switch (choice) {
            case 1:
                configureStrategy();
                break;
            case 2:
                configureDataSources();
                break;
            case 3:
                configureRiskParameters();
                break;
            case 4:
                configureAnalysisSettings();
                break;
            case 5:
                viewCurrentConfiguration();
                break;
            case 6:
                return;
            default:
                std::cout << "Invalid option. Please try again.\n";
                break;
        }
    }
}

void ConsoleUI::configureStrategy() {
    std::cout << "\n====== Strategy Configuration ======\n";
    
    // List available strategies
    std::cout << "Available strategies:\n";
    std::cout << "1. ORDER_BOOK_IMBALANCE\n";
    std::cout << "2. PAIRS_TRADING\n";
    // Add more strategies as needed
    
    std::cout << "Select a strategy (1-2): ";
    int strategyChoice;
    std::cin >> strategyChoice;
    std::cin.ignore(); // Clear newline
    
    std::string strategyName;
    switch (strategyChoice) {
        case 1:
            strategyName = "ORDER_BOOK_IMBALANCE";
            break;
        case 2:
            strategyName = "PAIRS_TRADING";
            break;
        default:
            std::cout << "Invalid choice. Using default (ORDER_BOOK_IMBALANCE).\n";
            strategyName = "ORDER_BOOK_IMBALANCE";
            break;
    }
    
    // Get symbol
    std::cout << "Enter trading symbol (e.g., BTCUSDT): ";
    std::string symbol;
    std::getline(std::cin, symbol);
    if (symbol.empty()) symbol = "BTCUSDT";
    
    // Strategy-specific parameters
    nlohmann::json params;
    if (strategyName == "ORDER_BOOK_IMBALANCE") {
        std::cout << "Enter lookback levels (default 10): ";
        std::string input;
        std::getline(std::cin, input);
        int lookbackLevels = input.empty() ? 10 : std::stoi(input);
        
        std::cout << "Enter imbalance threshold (default 1.5): ";
        std::getline(std::cin, input);
        double imbalanceThreshold = input.empty() ? 1.5 : std::stod(input);
        
        params = {
            {"lookback_levels", lookbackLevels},
            {"imbalance_threshold", imbalanceThreshold}
        };
    } else if (strategyName == "PAIRS_TRADING") {
        // Add parameters for pairs trading
        std::cout << "Enter z-score threshold (default 2.0): ";
        std::string input;
        std::getline(std::cin, input);
        double zScoreThreshold = input.empty() ? 2.0 : std::stod(input);
        
        params = {
            {"z_score_threshold", zScoreThreshold}
        };
    }
    
    // Update the configuration
    bool strategyExists = false;
    for (auto& strategy : config_["strategies"]) {
        if (strategy["name"] == strategyName) {
            strategy["symbol"] = symbol;
            strategy["params"] = params;
            strategy["active"] = true;
            strategyExists = true;
            break;
        }
    }
    
    if (!strategyExists) {
        config_["strategies"].push_back({
            {"name", strategyName},
            {"symbol", symbol},
            {"params", params},
            {"active", true}
        });
    }
    
    saveConfig();
    std::cout << "Strategy configuration updated.\n";
}

void ConsoleUI::configureDataSources() {
    std::cout << "\n====== Data Source Configuration ======\n";
    
    // Configure historical data
    std::cout << "Historical Data Configuration:\n";
    std::cout << "Enter start date (YYYY-MM-DD): ";
    std::string startDate;
    std::getline(std::cin, startDate);
    if (!startDate.empty()) config_["data"]["start_date"] = startDate;
    
    std::cout << "Enter end date (YYYY-MM-DD): ";
    std::string endDate;
    std::getline(std::cin, endDate);
    if (!endDate.empty()) config_["data"]["end_date"] = endDate;
    
    std::cout << "Enter trade data directory (default: data): ";
    std::string tradeDataDir;
    std::getline(std::cin, tradeDataDir);
    if (!tradeDataDir.empty()) config_["data"]["trade_data_dir"] = tradeDataDir;
    
    // Configure live data
    std::cout << "\nLive Data Configuration:\n";
    std::cout << "Enter WebSocket host (default: stream.binance.com): ";
    std::string host;
    std::getline(std::cin, host);
    if (!host.empty()) config_["data_handler"]["live_host"] = host;
    
    std::cout << "Enter WebSocket port (default: 9443): ";
    std::string port;
    std::getline(std::cin, port);
    if (!port.empty()) config_["data_handler"]["live_port"] = port;
    
    std::cout << "Enter WebSocket target (default: /ws/btcusdt@trade): ";
    std::string target;
    std::getline(std::cin, target);
    if (!target.empty()) config_["data_handler"]["live_target"] = target;
    
    saveConfig();
    std::cout << "Data source configuration updated.\n";
}

void ConsoleUI::configureRiskParameters() {
    std::cout << "\n====== Risk Parameter Configuration ======\n";
    
    std::cout << "Enter risk per trade (% of portfolio, default: 1.0): ";
    std::string input;
    std::getline(std::cin, input);
    if (!input.empty()) config_["risk"]["risk_per_trade_pct"] = std::stod(input) / 100.0;
    
    std::cout << "Enter maximum drawdown allowed (%, default: 20.0): ";
    std::getline(std::cin, input);
    if (!input.empty()) config_["risk"]["max_drawdown_pct"] = std::stod(input) / 100.0;
    
    std::cout << "Enter maximum position size (% of portfolio, default: 10.0): ";
    std::getline(std::cin, input);
    if (!input.empty()) config_["risk"]["max_position_size"] = std::stod(input) / 100.0;
    
    saveConfig();
    std::cout << "Risk parameters updated.\n";
}

void ConsoleUI::configureAnalysisSettings() {
    std::cout << "\n====== Analysis Configuration ======\n";
    
    // Ensure analytics section exists
    if (!config_.contains("analytics")) {
        config_["analytics"] = nlohmann::json::object();
    }
    
    std::cout << "Enter report directory (default: reports): ";
    std::string reportDir;
    std::getline(std::cin, reportDir);
    if (!reportDir.empty()) config_["analytics"]["report_dir"] = reportDir;
    
    // Configure metrics to track
    std::cout << "Select metrics to track (comma-separated):\n";
    std::cout << "1. Sharpe Ratio\n";
    std::cout << "2. Sortino Ratio\n";
    std::cout << "3. Maximum Drawdown\n";
    std::cout << "4. Win Rate\n";
    std::cout << "5. Profit Factor\n";
    std::cout << "Enter your choices (e.g., 1,3,4): ";
    
    std::string metricChoices;
    std::getline(std::cin, metricChoices);
    
    if (!metricChoices.empty()) {
        std::vector<std::string> metrics;
        size_t pos = 0;
        std::string token;
        
        while ((pos = metricChoices.find(',')) != std::string::npos) {
            token = metricChoices.substr(0, pos);
            int choice = std::stoi(token);
            
            switch (choice) {
                case 1: metrics.push_back("sharpe_ratio"); break;
                case 2: metrics.push_back("sortino_ratio"); break;
                case 3: metrics.push_back("max_drawdown"); break;
                case 4: metrics.push_back("win_rate"); break;
                case 5: metrics.push_back("profit_factor"); break;
            }
            
            metricChoices.erase(0, pos + 1);
        }
        
        // Process the last token
        if (!metricChoices.empty()) {
            int choice = std::stoi(metricChoices);
            switch (choice) {
                case 1: metrics.push_back("sharpe_ratio"); break;
                case 2: metrics.push_back("sortino_ratio"); break;
                case 3: metrics.push_back("max_drawdown"); break;
                case 4: metrics.push_back("win_rate"); break;
                case 5: metrics.push_back("profit_factor"); break;
            }
        }
        
        if (!metrics.empty()) {
            config_["analytics"]["metrics"] = metrics;
        }
    }
    
    saveConfig();
    std::cout << "Analysis configuration updated.\n";
}

void ConsoleUI::viewCurrentConfiguration() {
    std::cout << "\n====== Current Configuration ======\n";
    std::cout << std::setw(4) << config_ << std::endl;
    std::cout << "Press Enter to continue...";
    std::cin.get();
}