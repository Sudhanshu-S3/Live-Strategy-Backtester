cmake_minimum_required(VERSION 3.16)
project(Live_Strategy_Backtester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find System-Installed Dependencies ---
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

# --- Configure CPR to use system libraries to avoid build issues ---
set(CPR_USE_SYSTEM_CURL ON)

# --- Add Downloaded Libraries from Source ---
add_subdirectory(lib/cpr)
# Manually add zstd from its source, pointing to its correct build directory
add_subdirectory(lib/zstd/build/cmake)


# --- Project Sources ---
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
    src/strategy/MarketRegimeDetector.cpp
    src/strategy/MLStrategyClassifier.cpp
    src/strategy/NewsSentimentStrategy.cpp
    src/strategy/OrderBookImbalanceStrategy.cpp
    src/strategy/PairTradingStrategy.cpp
    src/strategy/SimpleMovingAverageCrossover.cpp
    src/strategy/StrategyFactory.cpp
)

# --- Executable ---
add_executable(backtester ${PROJECT_SOURCES})

target_compile_definitions(backtester PUBLIC 
    ASIO_STANDALONE
    WIN32_LEAN_AND_MEAN
)
# --- Include Directories ---
target_include_directories(backtester PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    # Header-only libraries
    "${PROJECT_SOURCE_DIR}/lib/nlohmann"
    "${PROJECT_SOURCE_DIR}/lib/websocketpp"
    "${PROJECT_SOURCE_DIR}/lib/asio/asio/include"
    # Add the mio library include path
    "${PROJECT_SOURCE_DIR}/lib/mio" 
    # Manually add the include directory for the zstd library
    "${PROJECT_SOURCE_DIR}/lib/zstd/lib"
)

# --- Link Libraries ---
target_link_libraries(backtester PRIVATE
    # System libraries
    Threads::Threads
    ${PostgreSQL_LIBRARIES}
    PkgConfig::PQXX

    # Found libraries
    cpr::cpr
    # Manually link to the actual static library target created by the zstd build script.
    # Note: It is NOT zstd::zstd, but libzstd_static.
    libzstd_static
)

# --- Compiler Options ---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(backtester PRIVATE -Wno-cpp)
endif()

# --- Installation ---
install(TARGETS backtester DESTINATION bin)