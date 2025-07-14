#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "DataTypes.h"
#include <vector>
#include <map>
#include <string>

using namespace std;

class Portfolio {
public:
    
    Portfolio(double initial_capital);


    void onFill(const FillEvent& fill);

    void updateTimeIndex(const string& symbol, const Bar& bar);

    double getInitialCapital() const;
    double getCurrentCash() const;
    double getTotalValue() const;
    const vector<pair<uint64_t, double>>& getEquityCurve() const;
    bool isHolding(const string& symbol) const;
    bool isLong(const string& symbol) const;

private:
    double initial_capital;
    double current_cash;
    map<string, double> holdings;
    map<string, double> current_holdings_value;
    vector<pair<uint64_t, double>> equity_curve; 
};

#endif
