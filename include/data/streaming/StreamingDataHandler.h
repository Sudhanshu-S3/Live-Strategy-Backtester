#ifndef STREAMING_DATA_HANDLER_H
#define STREAMING_DATA_HANDLER_H

#include "data/DataHandler.h"
#include "event/EventQueue.h"
#include <vector>
#include <string>
#include <thread>
#include <atomic>

class StreamingDataHandler : public DataHandler {
public:
    StreamingDataHandler(EventQueue& events, const std::vector<std::string>& symbols);
    ~StreamingDataHandler() override;

    void continue_backtest() override;
    void update_bars() override;

private:
    void stream_data();

    EventQueue& events_;
    std::vector<std::string> symbols_;
    std::thread data_thread_;
    std::atomic<bool> continue_streaming_;
};

#endif // STREAMING_DATA_HANDLER_H
