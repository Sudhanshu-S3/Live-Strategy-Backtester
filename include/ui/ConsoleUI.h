#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <nlohmann/json.hpp>
#include <memory>
#include "../core/Portfolio.h"

class ConsoleUI {
public:
    ConsoleUI();
    void displayMainMenu();
    void run(); // Declaring the run() method

private:
    void loadConfig();
    void saveConfig();
    
    // Main menu options
    void runBacktest();
    void runLiveShadowTrading();
    void compareLiveVsBacktest();
    void runOptimization();
    void runWalkForwardAnalysis();
    void runMonteCarloSimulation();
    
    // Configuration menu
    void configureSettings();
    void configureStrategy();
    void configureDataSources();
    void configureRiskParameters();
    void configureAnalysisSettings();
    void viewCurrentConfiguration();
    
    nlohmann::json config_;
    std::shared_ptr<Portfolio> last_backtest_portfolio_;
    std::shared_ptr<Portfolio> last_live_portfolio_;
};

#endif // CONSOLE_UI_H