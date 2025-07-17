#ifndef PERFORMANCE_FORECASTER_H
#define PERFORMANCE_FORECASTER_H

#include "../core/Portfolio.h"
#include <vector>
#include <string>
#include <memory>

class PerformanceForecaster {
public:
    PerformanceForecaster(const std::string& model_path);
    virtual ~PerformanceForecaster() = default;

    // Takes the historical equity curve and forecasts future values
    virtual std::vector<double> forecast_equity(const std::vector<double>& historical_equity, int future_periods);

    // A more complex forecast that might return multiple metrics
    struct ForecastResult {
        std::vector<double> equity_forecast;
        double predicted_sharpe;
        double predicted_max_drawdown;
    };
    virtual ForecastResult forecast_performance(const Portfolio& portfolio, int future_periods);

private:
    std::string model_path_;
    void load_model();
};

#endif // PERFORMANCE_FORECASTER_H 