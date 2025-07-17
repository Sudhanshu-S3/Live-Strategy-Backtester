#ifndef WEBSOCKET_DATA_HANDLER_H
#define WEBSOCKET_DATA_HANDLER_H

#include "DataHandler.h"
#include "event/Event.h" // Assuming Event.h exists in include/event/
#include "event/ThreadSafeQueue.h" // Assuming you have a thread-safe queue

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>

#include <string>
#include <vector>
#include <memory>
#include <thread>

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
        const std::string& target // e.g., "/ws/btcusdt@trade"
    );

    virtual ~WebSocketDataHandler();

    // Start the connection and I/O loop
    void connect();

    // Stop the connection
    void stop();
    
    // --- DataHandler Interface ---
    void updateBars() override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override;
    std::optional<OrderBook> getLatestOrderBook(const std::string& symbol) const override { return std::nullopt; } // Not implemented
    const std::vector<std::string>& getSymbols() const override { return symbols_; }
    void notifyOnNewData(std::function<void()> callback) override { on_new_data_ = callback; }

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_close(beast::error_code ec);

    void process_message(const std::string& message);

    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::vector<std::string> symbols_;
    std::atomic<bool> finished_{false};

    std::string host_;
    std::string port_;
    std::string target_;
    
    net::io_context ioc_;
    ssl::context ctx_{ssl::context::tlsv12_client};
    tcp::resolver resolver_{ioc_};
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_{ioc_, ctx_};
    beast::flat_buffer buffer_;

    std::thread ioc_thread_;
    std::map<std::string, Bar> latest_bars_map_; // For compatibility with original design
};

#endif // WEBSOCKET_DATA_HANDLER_H