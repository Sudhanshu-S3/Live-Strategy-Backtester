#ifndef WEBSOCKET_DATA_HANDLER_H
#define WEBSOCKET_DATA_HANDLER_H

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include "../data/DataHandler.h"
#include "../event/ThreadSafeQueue.h"
#include "../event/Event.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class WebSocketDataHandler : public DataHandler, public std::enable_shared_from_this<WebSocketDataHandler> {
public:
    WebSocketDataHandler(
        std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue,
        const std::vector<std::string>& symbols,
        const std::string& host,
        const std::string& port,
        const std::string& target
    );
    
    ~WebSocketDataHandler();
    
    // Connection management
    void connect();
    void stop();
    void setOnNewDataCallback(std::function<void()> callback);
    
    // DataHandler interface implementation
    void updateBars() override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override;
    
    // Implement missing pure virtual functions from DataHandler
    std::optional<OrderBook> getLatestOrderBook(const std::string& symbol) const override;
    const std::vector<std::string>& getSymbols() const override;
    void notifyOnNewData(std::function<void()> callback) override;
    
private:
    // WebSocket callbacks
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t);
    void on_read(beast::error_code ec, std::size_t);
    void on_close(beast::error_code ec);
    
    // Message processing
    void process_message(const std::string& message);
    
    // Member variables
    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::vector<std::string> symbols_;
    std::string host_;
    std::string port_;
    std::string target_;
    
    // Boost.Beast WebSocket components
    net::io_context ioc_;
    ssl::context ctx_{ssl::context::tlsv12_client};
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    
    // Thread management
    std::thread ioc_thread_;
    std::atomic<bool> finished_{true};
    
    // Data storage
    mutable std::unordered_map<std::string, Bar> latest_bars_map_;
    std::unordered_map<std::string, int> trade_counts_;
    
    // Callback for new data
    std::function<void()> on_new_data_;

    // Add data storage for order books
    mutable std::unordered_map<std::string, OrderBook> latest_orderbooks_;
<<<<<<< HEAD

    // Map to store orderbooks by symbol
    struct StoredOrderBook {
        std::map<double, std::pair<double, double>> bids;
        std::map<double, std::pair<double, double>> asks;
    };
    std::unordered_map<std::string, StoredOrderBook> orderbooks_;
=======
>>>>>>> ef82a6ae559d39c2be7a0dee4c6355537669c2a5
};

#endif // WEBSOCKET_DATA_HANDLER_H