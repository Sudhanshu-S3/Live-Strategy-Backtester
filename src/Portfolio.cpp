#include "Portfolio.h"
#include <numeric> 

using namespace std;


Portfolio::Portfolio(double initial_capital)
    : initial_capital(initial_capital), current_cash(initial_capital) {}

void Portfolio::onFill(const FillEvent& fill) {
    if (fill.direction == OrderDirection::BUY) {
        
        double cost = (fill.fill_price * fill.quantity);
        
        this->current_cash -= (cost + fill.commission);
        
        this->holdings[fill.symbol] += fill.quantity;
    } else if (fill.direction == OrderDirection::SELL) {
        
        double proceeds = (fill.fill_price * fill.quantity);
        
        this->current_cash += (proceeds - fill.commission);
        
        this->holdings[fill.symbol] -= fill.quantity;
    }
}

void Portfolio::updateTimeIndex(const string& symbol, const Bar& bar) {
    // Update the market value of the specific symbol
    if (holdings.count(symbol)) {
        current_holdings_value[symbol] = holdings.at(symbol) * bar.close;
    }

    // Calculate total market value across all holdings
    double total_market_value = 0.0;
    for (const auto& pair : current_holdings_value) {
        // Ensure we only use the value of assets we actually hold
        if (holdings.count(pair.first) && holdings.at(pair.first) != 0) {
            total_market_value += pair.second;
        }
    }

    // Calculate total portfolio value and record it
    double total_value = this->current_cash + total_market_value;
    this->equity_curve.push_back({bar.timestamp, total_value});
}



double Portfolio::getInitialCapital() const {
    return initial_capital;
}

double Portfolio::getCurrentCash() const {
    return current_cash;
}

double Portfolio::getTotalValue() const {
    
    if (equity_curve.empty()) {
        return initial_capital;
    }
    return equity_curve.back().second;
}

const vector<pair<uint64_t, double>>& Portfolio::getEquityCurve() const {
    return equity_curve;
}

bool Portfolio::isHolding(const string& symbol) const {
    auto it = holdings.find(symbol);
    if (it != holdings.end()) {
        // A position is held if the quantity is not zero
        return it->second != 0.0;
    }
    return false;
}

bool Portfolio::isLong(const string& symbol) const {
    auto it = holdings.find(symbol);
    if (it != holdings.end()) {
        // A position is long if the quantity is positive
        return it->second > 0.0;
    }
    return false;
}
