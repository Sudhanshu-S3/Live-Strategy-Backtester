#ifndef ORDER_BOOK_EVENT_H
#define ORDER_BOOK_EVENT_H

#include "Event.h"
#include "../data/DataTypes.h" // Include DataTypes.h to use OrderBookLevel
#include <string>
#include <vector>

class OrderBookEvent : public Event {
public:
    OrderBookEvent(const std::string& symbol, long long timestamp) 
        : Event(), symbol_(symbol), timestamp_(timestamp) {
        type = EventType::ORDER_BOOK; // Set the type after calling the base constructor
    }

    // Add constructor from OrderBook
    OrderBookEvent(const OrderBook& book)
        : Event(), symbol_(book.symbol), timestamp_(book.timestamp) {
        type = EventType::ORDER_BOOK;
        
        // Convert bids and asks from OrderBook to OrderBookLevel
        for (const auto& bid : book.bids) {
            bid_levels_.emplace_back(bid.first, bid.second);
        }
        
        for (const auto& ask : book.asks) {
            ask_levels_.emplace_back(ask.first, ask.second);
        }
    }

    void addBidLevel(double price, double quantity) {
        bid_levels_.emplace_back(price, quantity);
    }

    void addAskLevel(double price, double quantity) {
        ask_levels_.emplace_back(price, quantity);
    }

    // Add getters for the bid and ask levels (mentioned in error)
    const std::vector<OrderBookLevel>& getBidLevels() const {
        return bid_levels_;
    }

    const std::vector<OrderBookLevel>& getAskLevels() const {
        return ask_levels_;
    }

    std::string symbol_;
    long long timestamp_;
    std::vector<OrderBookLevel> bid_levels_;
    std::vector<OrderBookLevel> ask_levels_;
};

#endif // ORDER_BOOK_EVENT_H