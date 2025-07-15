#ifndef DATABASE_DATA_HANDLER_H
#define DATABASE_DATA_HANDLER_H

#include "DataHandler.h"
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>

class DatabaseDataHandler : public DataHandler {
public:
    DatabaseDataHandler(EventQueue& q, const std::string& connection_string,
                        const std::string& symbol, const std::string& start_date,
                        const std::string& end_date);
    ~DatabaseDataHandler();

    void get_next_event() override;

private:
    void load_chunk(); // Method to load the next batch of data

    std::unique_ptr<pqxx::connection> conn;
    std::string symbol;
    std::string start_date;
    std::string end_date;
    
    pqxx::result data_chunk;
    pqxx::result::const_iterator data_iterator;
    
    std::string last_loaded_timestamp;
    const int CHUNK_SIZE = 10000; // Load 10,000 events at a time
};

#endif // DATABASE_DATA_HANDLER_H