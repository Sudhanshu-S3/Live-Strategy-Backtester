# Use a modern CMake version
cmake_minimum_required(VERSION 3.20)
project(Live_Strategy_Backtester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find System-Installed Dependencies ---
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

# --- Configure CPR to use system libraries ---
set(CPR_USE_SYSTEM_CURL ON)

# --- Add Downloaded Libraries from Source ---
add_subdirectory(lib/cpr)
add_subdirectory(lib/zstd/build/cmake)

# --- Project Sources ---
# !! IMPORTANT !!
# Verify this list is complete. Based on your errors, you may be
# missing files like Performance.cpp, Portfolio.cpp, etc.
set(PROJECT_SOURCES
    src/main.cpp
    src/analytics/Analytics.cpp
    src/analytics/PerformanceForecaster.cpp
    src/core/Backtester.cpp
    src/core/MonteCarloSimulator.cpp
    src/core/Optimizer.cpp
    src/core/Performance.cpp
    src/core/Portfolio.cpp
    src/core/WalkForwardAnalyzer.cpp
    src/cross_asset_analysis/CrossAssetAnalyzer.cpp
    src/data/DatabaseDataHandler.cpp
    src/data/HFTDataHandler.cpp
    src/data/HistoricCSVDataHandler.cpp
    src/data/WebSocketDataHandler.cpp
    src/execution/SimulatedExecutionHandler.cpp
    src/risk/RiskManager.cpp
    src/risk/SharpeRatio.cpp
    src/strategy/MarketRegimeDetector.cpp
    src/strategy/MLStrategyClassifier.cpp
    src/strategy/NewsSentimentStrategy.cpp
    src/strategy/OrderBookImbalanceStrategy.cpp
    src/strategy/PairsTradingStrategy.cpp
    src/strategy/SimpleMovingAverageCrossover.cpp
    src/strategy/StrategyFactory.cpp
    src/ui/ConsoleUI.cpp
)

# --- Add the new source files
set(CONFIG_SOURCES
    src/config/AppConfig.cpp
)

# --- Executable ---
add_executable(backtester ${PROJECT_SOURCES} ${CONFIG_SOURCES})

# --- Include Directories ---
target_include_directories(backtester PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib/mio/single_include"
)

# --- Compile Definitions ---
target_compile_definitions(backtester PUBLIC 
    WIN32_LEAN_AND_MEAN
)

# --- Link Libraries ---
target_link_libraries(backtester PRIVATE
    # Boost libraries
    Boost::system
    Boost::thread
    # OpenSSL libraries
    OpenSSL::SSL
    OpenSSL::Crypto
    # Other libraries
    Threads::Threads
    ${PostgreSQL_LIBRARIES}
    PkgConfig::PQXX
    cpr::cpr
    libzstd_static
    nlohmann_json::nlohmann_json

    # --- FIX FOR LINKER ERRORS ---
    # Add the Windows Sockets library
    ws2_32
)

# --- Compiler Options ---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(backtester PRIVATE -Wno-cpp)
endif()

# --- Installation ---
install(TARGETS backtester DESTINATION bin)

# Add the ConfigValidator tool
add_executable(config_validator src/tools/ConfigValidator.cpp)
target_link_libraries(config_validator PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(config_validator PRIVATE ${CMAKE_SOURCE_DIR}/include)


# If you need to link against other backtester code objects, do something like this:
# target_link_libraries(strategy_tester PRIVATE backtester_core)