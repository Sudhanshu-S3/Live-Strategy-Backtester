#include "../../include/risk/RiskManager.h" 
#include "../../include/event/Event.h"
#include <cmath>
#include <iostream>
#include <iomanip> // <--- ADD THIS LINE for std::setprecision
#include <memory> 
#include <numeric>
#include <vector> 

// Helper function to convert OrderDirection enum to string for logging
std::string orderDirectionToString(OrderDirection dir) {
    switch (dir) {
        case OrderDirection::BUY: return "BUY";
        case OrderDirection::SELL: return "SELL";
        case OrderDirection::NONE: return "NONE"; 
        default: return "UNKNOWN";
    }
}

RiskManager::RiskManager(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
                         std::shared_ptr<Portfolio> portfolio, 
                         const nlohmann::json& risk_config) : 
    event_queue_(event_queue), 
    portfolio_(portfolio)
{
    thresholds_.max_drawdown_pct = risk_config.value("max_drawdown_pct", 0.20);
    thresholds_.daily_var_95_pct = risk_config.value("daily_var_95_pct", 0.05);
    thresholds_.portfolio_loss_threshold_pct = risk_config.value("portfolio_loss_threshold_pct", 0.10);

    use_volatility_sizing_ = risk_config.value("use_volatility_sizing", false);
    risk_per_trade_pct_ = risk_config.value("risk_per_trade_pct", 0.01);
    volatility_lookback_ = risk_config.value("volatility_lookback", 20);
}

void RiskManager::onSignal(const SignalEvent& signal) {
    if (trading_halted_) {
        std::cout << "RISK ALERT: Trading halted. Ignoring signal for " << signal.symbol << std::endl;
        return;
    }

    double total_equity = portfolio_->get_total_equity();
    // Assumes Portfolio has a public get_cash() method
    double cash = portfolio_->get_cash(); 
    double last_price = portfolio_->get_last_price(signal.symbol);

    if (last_price <= 0) {
        std::cerr << "RiskManager: Could not get last price for " << signal.symbol << ". Order rejected." << std::endl;
        return;
    }

    double quantity = 0;
    if (use_volatility_sizing_) {
        double volatility = calculateVolatility(signal.symbol);
        if (volatility > 1e-6) {
            double risk_amount = total_equity * risk_per_trade_pct_;
            quantity = risk_amount / (volatility * last_price);
        } else {
            std::cerr << "RiskManager: Volatility is zero for " << signal.symbol << ". Using fixed sizing." << std::endl;
            quantity = (total_equity * risk_per_trade_pct_) / last_price;
        }
    } else {
        quantity = (total_equity * risk_per_trade_pct_) / last_price;
    }

    if (quantity * last_price > cash) {
        quantity = cash / last_price * 0.99; 
    }

    if (quantity > 0) {
        // --- FIX: Argument order and types corrected for OrderEvent constructor
        auto order = std::make_shared<OrderEvent>(signal.symbol, signal.timestamp, signal.direction, quantity, OrderType::MARKET, signal.strategy_name);

        // FIX: Wrap the event in another shared_ptr to match the queue type.
        // This seems to be a design pattern in this project, though it is unusual.
        if (event_queue_) {
            event_queue_->push(std::make_shared<std::shared_ptr<Event>>(order));
        }
    }
}

void RiskManager::onDataSourceStatus(const DataSourceStatusEvent& event) {
    std::string status_str;
    switch (event.status) {
        case DataSourceStatus::CONNECTED: status_str = "CONNECTED"; break;
        case DataSourceStatus::DISCONNECTED: status_str = "DISCONNECTED"; break;
        case DataSourceStatus::RECONNECTING: status_str = "RECONNECTING"; break;
        case DataSourceStatus::FALLBACK_ACTIVE: status_str = "FALLBACK_ACTIVE"; break;
    }
    std::cout << "RISK MANAGER: Data source status changed to " << status_str 
              << ". Message: " << event.message << std::endl;
}

void RiskManager::monitorRealTimeRisk() {
    if (trading_halted_) return;

    // Assumes Portfolio has a public get_max_drawdown() method
    double current_max_drawdown = portfolio_->get_max_drawdown(); 
    if (current_max_drawdown > thresholds_.max_drawdown_pct) {
        sendAlert("CRITICAL ALERT: Max Drawdown Exceeded! Current: " + 
                  std::to_string(current_max_drawdown * 100) + "% | Threshold: " + 
                  std::to_string(thresholds_.max_drawdown_pct * 100) + "%");
    } else {
        std::cout << "Current Max Drawdown: " << std::fixed << std::setprecision(2) 
                  << current_max_drawdown * 100 << "% (Below threshold)" << std::endl;
    }

    Performance current_performance = portfolio_->getRealTimePerformance();
    double current_var_95 = current_performance.calculateVaR(0.95);

    if (current_var_95 > thresholds_.daily_var_95_pct) {
        sendAlert("HIGH ALERT: Daily VaR (95%) Exceeded! Current: " + 
                  std::to_string(current_var_95 * 100) + "% | Threshold: " + 
                  std::to_string(thresholds_.daily_var_95_pct * 100) + "%");
    } else {
        std::cout << "Current VaR (95%): " << std::fixed << std::setprecision(2) 
                  << current_var_95 * 100 << "% (Below threshold)" << std::endl;
    }

    std::map<std::string, Position> current_positions = portfolio_->getCurrentPositions();
    if (!current_positions.empty()) {
        std::cout << "Current Open Positions:" << std::endl;
        for (const auto& pair : current_positions) {
            const Position& pos = pair.second;
            std::cout << "  " << pos.symbol << ": Quantity=" << pos.quantity 
                      << ", Avg Cost=" << std::fixed << std::setprecision(2) << pos.average_cost
                      << ", Market Value=" << std::fixed << std::setprecision(2) << pos.market_value 
                      << ", Direction=" << orderDirectionToString(pos.direction) << std::endl;
        }
    } else {
        std::cout << "No open positions." << std::endl;
    }
    std::cout << "--------------------------------------\n";

    double current_equity = portfolio_->get_total_equity();
    double initial_capital = portfolio_->getInitialCapital();
    double loss_pct = (initial_capital - current_equity) / initial_capital;

    if (loss_pct > thresholds_.portfolio_loss_threshold_pct) {
        trading_halted_ = true;
        sendAlert("CRITICAL: PORTFOLIO CIRCUIT BREAKER TRIPPED! TRADING HALTED.");
    }
}

void RiskManager::sendAlert(const std::string& message) {
    std::cerr << "!!!!! RISK ALERT !!!!! " << message << std::endl;
    // auto alert_event = std::make_shared<Event>(); 
    // event_queue_->push(alert_event);
}

double RiskManager::calculateVolatility(const std::string& symbol) {
    // Assumes Portfolio has a public get_latest_bars() method
    auto prices_bar = portfolio_->get_latest_bars(symbol, volatility_lookback_); 
    if (prices_bar.size() < volatility_lookback_) {
        return 0.0; 
    }

    std::vector<double> returns;
    for (size_t i = 1; i < prices_bar.size(); ++i) {
        returns.push_back(log(prices_bar[i].close / prices_bar[i-1].close));
    }

    if (returns.size() < 2) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / returns.size() - mean * mean);
    
    return stdev;
};