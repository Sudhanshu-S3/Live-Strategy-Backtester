#ifndef DATABASE_DATA_HANDLER_H
#define DATABASE_DATA_HANDLER_H

#include "DataHandler.h"
#include "../event/ThreadSafeQueue.h"
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>
#include <optional>

class DatabaseDataHandler : public DataHandler {
public:
    DatabaseDataHandler(std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue, 
                        const std::string& connection_string,
                        const std::vector<std::string>& symbols, 
                        const std::string& start_date,
                        const std::string& end_date);
    ~DatabaseDataHandler();

    // Interface methods
    void updateBars() override;
    bool isFinished() const override;
    std::optional<Bar> getLatestBar(const std::string& symbol) const override;
    double getLatestBarValue(const std::string& symbol, const std::string& val_type) override;
    std::vector<Bar> getLatestBars(const std::string& symbol, int n = 1) override;

private:
    void load_chunk(const std::string& symbol); // Method to load the next batch of data for a symbol

    std::shared_ptr<ThreadSafeQueue<std::shared_ptr<Event>>> event_queue_;
    std::unique_ptr<pqxx::connection> conn;
    std::vector<std::string> symbols_;
    std::string start_date_;
    std::string end_date_;
    
    // Per-symbol data chunks and iterators
    std::map<std::string, pqxx::result> data_chunks_;
    std::map<std::string, pqxx::result::const_iterator> data_iterators_;
    
    std::string last_loaded_timestamp_;
    const int CHUNK_SIZE = 10000; // Load 10,000 events at a time
};

#endif // DATABASE_DATA_HANDLER_H