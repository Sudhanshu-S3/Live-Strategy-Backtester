#include "gtest/gtest.h"
#include "core/Portfolio.h"
#include "event/Event.h"
#include "data/DataHandler.h"
#include <memory>
#include <queue>

// Mock DataHandler for testing
class MockDataHandler : public DataHandler {
public:
    // Implement virtual functions...
    void updateBars(std::queue<std::shared_ptr<Event>>& q) override {}
    bool isFinished() const override { return true; }
    std::optional<Bar> getLatestBar(const std::string& symbol) const override { return std::nullopt; }
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override { return 105.0; }
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override { return {}; }
    const std::vector<std::string>& getSymbols() const override { static std::vector<std::string> s = {"TEST"}; return s; }
};

// STAGE 8: Example Test Fixture for Portfolio
class PortfolioTest : public ::testing::Test {
protected:
    std::shared_ptr<std::queue<std::shared_ptr<Event>>> event_queue;
    std::shared_ptr<DataHandler> data_handler;
    std::shared_ptr<Portfolio> portfolio;

    void SetUp() override {
        event_queue = std::make_shared<std::queue<std::shared_ptr<Event>>>();
        data_handler = std::make_shared<MockDataHandler>();
        portfolio = std::make_shared<Portfolio>(event_queue, data_handler, 100000.0);
    }
};

TEST_F(PortfolioTest, InitialCapitalIsSet) {
    ASSERT_EQ(portfolio->getInitialCapital(), 100000.0);
}

TEST_F(PortfolioTest, OnFillUpdatesHoldingsAndCash) {
    FillEvent fill("2023-01-01", "TEST", "EXCH", OrderDirection::BUY, 10, 100.0, 5.0);
    portfolio->onFill(fill);

    auto holdings = portfolio->getCurrentPositions();
    EXPECT_EQ(holdings["TEST"].quantity, 10);
    EXPECT_EQ(holdings["TEST"].avg_price, 100.0);
    
    // Initial Capital - (10 * 100) - 5.0 commission
    EXPECT_EQ(portfolio->getCash(), 100000.0 - 1000.0 - 5.0);
}

TEST_F(PortfolioTest, PortfolioValueUpdatesOnMarket) {
    FillEvent fill("2023-01-01", "TEST", "EXCH", OrderDirection::BUY, 10, 100.0, 0.0);
    portfolio->onFill(fill);

    // MockDataHandler returns 105.0 as the latest price
    portfolio->updatePortfolioValue(); 
    
    // Holdings value = 10 * 105 = 1050
    // Cash = 100000 - 1000 = 99000
    // Total Equity = 99000 + 1050 = 100050
    EXPECT_EQ(portfolio->getTotalEquity(), 100050.0);
}