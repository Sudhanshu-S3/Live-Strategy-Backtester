#include "../../include/analytics/Analytics.h"
#include <iostream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <windows.h>
#include <psapi.h>

// Helper function to convert VolatilityLevel enum to string
std::string volatilityLevelToString(VolatilityLevel level) {
    switch (level) {
        case VolatilityLevel::LOW: return "LOW";
        case VolatilityLevel::HIGH: return "HIGH";
        default: return "UNKNOWN";
    }
}

// Helper function for correlation calculation
double calculate_mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

double calculate_stddev(const std::vector<double>& v, double mean) {
    if (v.size() < 2) return 0.0;
    double sq_sum = 0.0;
    for (double val : v) {
        sq_sum += (val - mean) * (val - mean);
    }
    return std::sqrt(sq_sum / (v.size() - 1));
}

double calculate_correlation(const std::vector<double>& pnl1, const std::vector<double>& pnl2) {
    if (pnl1.size() != pnl2.size() || pnl1.size() < 2) {
        return 0.0; // Not enough data or mismatched sizes
    }
    double mean1 = calculate_mean(pnl1);
    double mean2 = calculate_mean(pnl2);
    double stddev1 = calculate_stddev(pnl1, mean1);
    double stddev2 = calculate_stddev(pnl2, mean2);

    if (stddev1 < 1e-9 || stddev2 < 1e-9) {
        return 0.0; // No variance, cannot calculate correlation
    }

    double covariance = 0.0;
    for (size_t i = 0; i < pnl1.size(); ++i) {
        covariance += (pnl1[i] - mean1) * (pnl2[i] - mean2);
    }
    covariance /= (pnl1.size() - 1);

    return covariance / (stddev1 * stddev2);
}


Analytics::Analytics(const nlohmann::json& analytics_config) {
    enable_cross_correlation_ = analytics_config.value("enable_cross_correlation", false);
    anomaly_z_score_threshold_ = analytics_config.value("anomaly_detection_z_score", 3.0);
}

void Analytics::generateReport(std::shared_ptr<Portfolio> portfolio) {
    std::cout << "\n--- Advanced Analytics Report ---\n";
    if (enable_cross_correlation_) {
        calculate_cross_correlations(portfolio);
    }
    std::cout << "---------------------------------\n";
}

void Analytics::calculate_cross_correlations(std::shared_ptr<Portfolio> portfolio) {
    std::cout << "\n--- Cross-Strategy Correlation Analysis ---\n";
    const auto& trade_log_by_strategy = portfolio->getStrategyTradeLog();

    if (trade_log_by_strategy.size() < 2) {
        std::cout << "Not enough strategies with trades to calculate correlations.\n";
        return;
    }

    // Step 1: Generate PnL series for each strategy
    std::map<std::string, std::vector<double>> pnl_series;
    for (const auto& [strategy_name, trades] : trade_log_by_strategy) {
        // This is a simplified PnL calculation. A proper implementation needs to match
        // entry and exit trades to calculate realized PnL for each complete transaction.
        // For now, we treat each trade's outcome independently, which is not realistic
        // but serves as a placeholder for the correlation logic.
        for (const auto& trade : trades) {
             // A more realistic approach would require trade matching (e.g., FIFO)
             // to calculate PnL. This is a complex task itself.
             // We'll use a conceptual PnL for now. Let's assume trade.pnl is populated.
             pnl_series[strategy_name].push_back(trade.pnl);
        }
    }

    // Align PnL series to the same length by padding with zeros
    size_t max_len = 0;
    for (const auto& [name, pnl] : pnl_series) {
        if (pnl.size() > max_len) {
            max_len = pnl.size();
        }
    }
    for (auto& [name, pnl] : pnl_series) {
        pnl.resize(max_len, 0.0);
    }

    // Step 2: Calculate and display the correlation matrix
    std::vector<std::string> strategy_names;
    for (const auto& [name, pnl] : pnl_series) {
        strategy_names.push_back(name);
    }
    
    // Print header
    std::cout << std::left << std::setw(25) << "Strategy";
    for (const auto& name : strategy_names) {
        std::cout << std::setw(15) << name.substr(0, 14);
    }
    std::cout << std::endl;

    for (size_t i = 0; i < strategy_names.size(); ++i) {
        std::cout << std::left << std::setw(25) << strategy_names[i].substr(0, 24);
        for (size_t j = 0; j < strategy_names.size(); ++j) {
            double corr = calculate_correlation(pnl_series[strategy_names[i]], pnl_series[strategy_names[j]]);
            std::cout << std::fixed << std::setprecision(3) << std::setw(15) << corr;
        }
        std::cout << std::endl;
    }

    std::cout << "------------------------------------------------\n";
}

void Analytics::comparePerformance(std::shared_ptr<Portfolio> live_portfolio, std::shared_ptr<Portfolio> backtest_portfolio) {
    if (!live_portfolio || !backtest_portfolio) {
        std::cerr << "Cannot compare performance: one or both portfolios are null." << std::endl;
        return;
    }

    // --- Create Performance objects ---
    auto get_equity_values = [](const auto& equity_curve) {
        std::vector<double> values;
        values.reserve(equity_curve.size());
        for (const auto& entry : equity_curve) {
            values.push_back(std::get<1>(entry));
        }
        return values;
    };

    Performance live_perf(get_equity_values(live_portfolio->getEquityCurve()), live_portfolio->getInitialCapital());
    Performance backtest_perf(get_equity_values(backtest_portfolio->getEquityCurve()), backtest_portfolio->getInitialCapital());

    // --- Print Comparison Report ---
    std::cout << "\n--- Live vs. Backtest Performance Comparison ---\n";
    printf("%-20s | %-15s | %-15s\n", "Metric", "Live", "Backtest");
    printf("%-20s | %-15.2f | %-15.2f\n", "Total Return (%)", live_perf.getTotalReturn() * 100, backtest_perf.getTotalReturn() * 100);
    printf("%-20s | %-15.2f | %-15.2f\n", "Max Drawdown (%)", live_perf.getMaxDrawdown() * 100, backtest_perf.getMaxDrawdown() * 100);
    printf("%-20s | %-15.3f | %-15.3f\n", "Sharpe Ratio", live_perf.getSharpeRatio(), backtest_perf.getSharpeRatio());
    printf("%-20s | %-15.2f | %-15.2f\n", "VaR (95%%)", live_perf.calculateVaR(0.95), backtest_perf.calculateVaR(0.95));
    std::cout << "--------------------------------------------------\n";
}

void Analytics::generateMarketConditionReport(std::shared_ptr<Portfolio> portfolio) {
    std::cout << "\n--- Market Condition Performance Breakdown ---\n";

    std::map<VolatilityLevel, std::vector<Trade>> trades_by_vol;
    std::map<TrendDirection, std::vector<Trade>> trades_by_trend;

    for (const auto& trade : portfolio->getTradeLog()) {
        trades_by_vol[trade.market_state_at_entry.volatility].push_back(trade);
        trades_by_trend[trade.market_state_at_entry.trend].push_back(trade);
    }

    std::cout << "\n--- Performance by Volatility ---\n";
    for (const auto& [vol, trades] : trades_by_vol) {
        // PnL calculation would require matching entry/exit trades, which is not yet implemented.
        // For now, we just count trades.
        std::cout << "Volatility: " << volatilityLevelToString(vol) << ", Trades: " << trades.size() << std::endl;
    }

    std::cout << "\n--- Performance by Trend ---\n";
    // Helper function to convert TrendDirection enum to string
    auto trendDirectionToString = [](TrendDirection dir) -> std::string {
        switch (dir) {
            case TrendDirection::SIDEWAYS :  return "SIDEWAYS";
            default: return "UNKNOWN";
        }
    };

    for (const auto& [trend, trades] : trades_by_trend) {
        std::cout << "Trend: " << trendDirectionToString(trend) << ", Trades: " << trades.size() << std::endl;
    }

    std::cout << "--------------------------------------------\n";
}

void Analytics::generateFactorAnalysisReport(std::shared_ptr<Portfolio> portfolio) {
    std::cout << "\n--- Factor Exposure Analysis ---\n";
    const auto& equity_curve = portfolio->getEquityCurve();
    if (equity_curve.size() < 2) {
        std::cout << "Not enough data for factor analysis.\n";
        return;
    }

    // Step 1: Calculate portfolio returns
    std::vector<double> returns;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        double prev_equity = std::get<1>(equity_curve[i-1]);
        double curr_equity = std::get<1>(equity_curve[i]);
        if (prev_equity > 1e-9) {
            returns.push_back((curr_equity - prev_equity) / prev_equity);
        } else {
            returns.push_back(0.0);
        }
    }

    // Step 2: Create "factor" series from market state
    std::vector<double> vol_factor;
    std::vector<double> trend_factor;
    for (size_t i = 1; i < equity_curve.size(); ++i) {
        const auto& state = std::get<2>(equity_curve[i]);
        vol_factor.push_back(state.volatility_value); 
        trend_factor.push_back(static_cast<double>(state.trend)); // UP=1, DOWN=-1, SIDEWAYS=0
    }

    // Step 3: Calculate correlation (exposure) to these internal factors
    double vol_exposure = calculate_correlation(returns, vol_factor);
    double trend_exposure = calculate_correlation(returns, trend_factor);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Exposure to Volatility Factor: " << vol_exposure << std::endl;
    std::cout << "Exposure to Trend Factor:    " << trend_exposure << std::endl;
    std::cout << "\nInterpretation:\n";
    std::cout << " - Positive Volatility Exposure suggests the strategy performs better in high-volatility environments.\n";
    std::cout << " - Positive Trend Exposure suggests the strategy is trend-following.\n";
    std::cout << " - Negative Trend Exposure suggests the strategy is mean-reverting.\n";
    std::cout << "----------------------------------------------------------\n";
}

void Analytics::logDeployment(bool success) {
    if (success) {
        successful_deployments_++;
    } else {
        failed_deployments_++;
    }
}

void Analytics::generateDeploymentReport() {
    std::cout << "\n--- Strategy Deployment Report ---\n";
    int total_attempts = successful_deployments_ + failed_deployments_;
    if (total_attempts == 0) {
        std::cout << "No strategy deployments were attempted.\n";
    } else {
        double success_rate = static_cast<double>(successful_deployments_) / total_attempts * 100.0;
        std::cout << "Total Deployment Attempts: " << total_attempts << std::endl;
        std::cout << "Successful Deployments: " << successful_deployments_ << std::endl;
        std::cout << "Failed Deployments: " << failed_deployments_ << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << success_rate << "%\n";
    }
    std::cout << "----------------------------------\n";
}

void Analytics::snapshotSystemResources() {
    // Memory Usage
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        memory_usage_.push_back(pmc.PrivateUsage);
    }

    // CPU Usage
    static ULARGE_INTEGER last_cpu, last_sys_cpu, last_user_cpu;
    static int num_processors = -1;
    if (num_processors == -1) {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        num_processors = sys_info.dwNumberOfProcessors;
    }

    ULARGE_INTEGER now, sys, user;
    GetSystemTimeAsFileTime((FILETIME*)&now);

    HANDLE process = GetCurrentProcess();
    FILETIME ftime, fsys, fuser;
    GetProcessTimes(process, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    
    double percent = 0.0;
    if (last_cpu.QuadPart != 0) {
        percent = (double)((sys.QuadPart - last_sys_cpu.QuadPart) + (user.QuadPart - last_user_cpu.QuadPart));
        percent /= (now.QuadPart - last_cpu.QuadPart);
        percent /= num_processors;
    }
    last_cpu = now;
    last_user_cpu = user;
    last_sys_cpu = sys;
    
    cpu_usage_.push_back(percent * 100.0);
}

void Analytics::generateResourceUsageReport() {
    std::cout << "\n--- System Resource Usage Report ---\n";

    if (memory_usage_.empty() || cpu_usage_.empty()) {
        std::cout << "No resource usage data collected.\n";
    } else {
        double avg_mem = std::accumulate(memory_usage_.begin(), memory_usage_.end(), 0.0) / memory_usage_.size();
        size_t peak_mem = *std::max_element(memory_usage_.begin(), memory_usage_.end());

        double avg_cpu = std::accumulate(cpu_usage_.begin(), cpu_usage_.end(), 0.0) / cpu_usage_.size();
        double peak_cpu = *std::max_element(cpu_usage_.begin(), cpu_usage_.end());
        
        std::cout << "Average Memory Usage: " << avg_mem / (1024 * 1024) << " MB\n";
        std::cout << "Peak Memory Usage: " << (double)peak_mem / (1024 * 1024) << " MB\n";
        std::cout << "Average CPU Usage: " << std::fixed << std::setprecision(2) << avg_cpu << "%\n";
        std::cout << "Peak CPU Usage: " << std::fixed << std::setprecision(2) << peak_cpu << "%\n";
    }
    
    std::cout << "------------------------------------\n";
}


void Analytics::detect_anomalies(std::shared_ptr<DataHandler> data_handler) {
    if (anomaly_z_score_threshold_ <= 0) return;

    for(const auto& symbol : data_handler->getSymbols()) {
        double price = data_handler->getLatestBarValue(symbol, "price");
        if(price <= 0) continue;

        price_history_[symbol].push_back(price);
        if(price_history_[symbol].size() > anomaly_lookback_) {
            price_history_[symbol].erase(price_history_[symbol].begin());
        }
        
        if(price_history_[symbol].size() < anomaly_lookback_) continue;

        // Calculate z-score for the latest price
        double sum = std::accumulate(price_history_[symbol].begin(), price_history_[symbol].end(), 0.0);
        double mean = sum / anomaly_lookback_;
        double sq_sum = std::inner_product(price_history_[symbol].begin(), price_history_[symbol].end(), price_history_[symbol].begin(), 0.0);
        double std_dev = std::sqrt(sq_sum / anomaly_lookback_ - mean * mean);

        if(std_dev > 1e-9){
            double z_score = (price - mean) / std_dev;
            if(std::abs(z_score) > anomaly_z_score_threshold_){
                std::cerr << "!!! MARKET ANOMALY DETECTED !!! Symbol: " << symbol 
                          << ", Price: " << price << ", Z-Score: " << z_score << std::endl;
            }
        }
    }
}