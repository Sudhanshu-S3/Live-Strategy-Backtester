#ifndef WEBSOCKET_DATA_HANDLER_H
#define WEBSOCKET_DATA_HANDLER_H

#include "DataHandler.h"
#include "../event/ThreadSafeQueue.h"
#include "../event/Event.h" 
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <thread>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

typedef websocketpp::client<websocketpp::config::asio_client> client;

// WebSocketDataHandler connects to a live WebSocket feed for real-time market data.
class WebSocketDataHandler : public DataHandler {
public:
    WebSocketDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, const std::string& ws_uri, const std::vector<std::string>& symbols);
    ~WebSocketDataHandler() override;

    // Implementation of the DataHandler interface
    void updateBars() override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override;

private:
    void run();
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);

    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::string uri_;
    std::vector<std::string> symbols_;
    std::unique_ptr<client> ws_client_;
    std::thread ws_thread_;
    
    bool finished_ = false;
    std::map<std::string, Bar> latest_bars_map_;
};

#endif // WEBSOCKET_DATA_HANDLER_H