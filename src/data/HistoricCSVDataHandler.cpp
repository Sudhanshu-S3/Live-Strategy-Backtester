#include "../../include/data/HistoricCSVDataHandler.h"
#include "../../include/event/EventQueue.h" // Add this line to include EventQueue definition
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <iterator>
#include <string>
#include <map>
#include <future>
#include <thread>
#include <../../lib/mio/mio.hpp>

using namespace std;

// Forward declare the optimized parser
void parse_line_from_mmap(const char*& cursor, const std::string& symbol, std::vector<Bar>& bars_for_symbol);

HistoricCSVDataHandler::HistoricCSVDataHandler(EventQueue& events, std::string csv_dir, std::vector<std::string> symbols)
    : DataHandler(), csv_dir_(std::move(csv_dir)), symbols_(std::move(symbols)) {
    for (const auto& symbol : symbols_) {
        open_and_map_csv(symbol);
    }
}

void HistoricCSVDataHandler::open_and_map_csv(const std::string& symbol) {
    std::string filepath = csv_dir_ + "/" + symbol + ".csv";
    mio::mmap_source mmap;
    std::error_code error;
    mmap.map(filepath, error);
    if (error) {
        throw std::runtime_error("Could not map file: " + filepath);
    }
    mapped_files_.emplace(symbol, std::move(mmap));
    file_cursors_.emplace(symbol, mapped_files_.at(symbol).data());
    // Skip header
    const char* end_of_line = strchr(file_cursors_.at(symbol), '\n');
    if (end_of_line) {
        file_cursors_.at(symbol) = end_of_line + 1;
    }
}
/*
void HistoricCSVDataHandler::update_bars() {
    for (const auto& symbol : symbols_) {
        if (file_cursors_.at(symbol) < mapped_files_.at(symbol).data() + mapped_files_.at(symbol).size()) {
            std::vector<Bar> bars;
            parse_line_from_mmap(file_cursors_.at(symbol), symbol, bars);
            if (!bars.empty()) {
                events_.push(std::make_shared<MarketEvent>(bars[0]));
            }
        }
    }
}
*/

void HistoricCSVDataHandler::continue_backtest() {
    // This method might not be needed if update_bars handles everything
    // or it could be used to signal the end of the backtest.
}

void parse_line_from_mmap(const char*& cursor, const std::string& symbol, std::vector<Bar>& bars_for_symbol) {
    if (*cursor == '\0') return;

    const char* line_start = cursor;
    const char* line_end = strchr(line_start, '\n');
    if (!line_end) {
        line_end = line_start + strlen(line_start);
    }

    std::string line(line_start, line_end - line_start);
    cursor = (*line_end == '\n') ? line_end + 1 : line_end;

    if (line.empty()) return;

    Bar bar;
    bar.symbol = symbol;
    size_t start = 0;
    size_t end = line.find(',');

    bar.timestamp = line.substr(start, end - start);
    start = end + 1;

    end = line.find(',', start);
    bar.open = std::stod(line.substr(start, end - start));
    start = end + 1;

    end = line.find(',', start);
    bar.high = std::stod(line.substr(start, end - start));
    start = end + 1;

    end = line.find(',', start);
    bar.low = std::stod(line.substr(start, end - start));
    start = end + 1;

    end = line.find(',', start);
    bar.close = std::stod(line.substr(start, end - start));
    start = end + 1;

    bar.volume = std::stoll(line.substr(start));

    bars_for_symbol.push_back(bar);
}


// --- Interface Implementations ---

bool HistoricCSVDataHandler::isFinished() const {
    for (const auto& pair : current_bar_iterators) {
        const auto& symbol = pair.first;
        if (pair.second != all_bars.at(symbol).cend()) {
            return false;
        }
    }
    return true;
}

optional<Bar> HistoricCSVDataHandler::getLatestBar(const string& symbol) const {
    auto it = latest_bars_map.find(symbol);
    if (it != latest_bars_map.end()) {
        return it->second;
    }
    return nullopt;
}

double HistoricCSVDataHandler::getLatestBarValue(const string& symbol, const string& val_type) {
    auto it = latest_bars_map.find(symbol);
    if (it != latest_bars_map.end()) {
        const Bar& bar = it->second;
        if (val_type == "price" || val_type == "close") return bar.close;
        if (val_type == "open") return bar.open;
        if (val_type == "high") return bar.high;
        if (val_type == "low") return bar.low;
        if (val_type == "volume") return bar.volume;
    }
    return 0.0;
}

vector<Bar> HistoricCSVDataHandler::getLatestBars(const string& symbol, int n) {
    auto it = all_bars.find(symbol);
    if (it == all_bars.end()) {
        return {};
    }

    auto current_it = current_bar_iterators.at(symbol);
    auto start_it = all_bars.at(symbol).cbegin();

    auto distance = std::distance(start_it, current_it);
    if (distance < n) {
        return vector<Bar>(start_it, current_it);
    }
    
    return vector<Bar>(std::prev(current_it, n), current_it);
}

void HistoricCSVDataHandler::updateBars() {
    if (isFinished()) {
        return;
    }

    string next_symbol_to_process = "";
    string earliest_time = "9999-99-99";

    for (const auto& pair : current_bar_iterators) {
        const auto& symbol = pair.first;
        const auto& iterator = pair.second;

        if (iterator != all_bars.at(symbol).cend()) {
            if (iterator->timestamp < earliest_time) {
                earliest_time = iterator->timestamp;
                next_symbol_to_process = symbol;
            }
        }
    }

    if (!next_symbol_to_process.empty()) {
        const auto& bar_to_process = *current_bar_iterators.at(next_symbol_to_process);
        auto market_event = make_shared<MarketEvent>(bar_to_process.symbol, bar_to_process.timestamp);
        event_queue_->push(std::make_shared<std::shared_ptr<Event>>(market_event));

        latest_bars_map[next_symbol_to_process] = bar_to_process;

        current_bar_iterators.at(next_symbol_to_process)++;
    }
}