cmake_minimum_required(VERSION 3.16)
project(Live_Strategy_Backtester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---
# Find PostgreSQL's C library (libpq)
find_package(PostgreSQL REQUIRED)

# Find libpqxx, the C++ library
# On many systems, pkg-config can find this automatically
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

# Find nlohmann_json
add_subdirectory(lib/nlohmann)

# --- Include Directories ---
include_directories(
    include
    ${PostgreSQL_INCLUDE_DIRS} # Add PostgreSQL includes
)
include(FetchContent)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5 # Use a specific tag for stability
)
FetchContent_MakeAvailable(cpr)
# --- Add Executable ---
add_executable(backtester
    src/core/Backtester.cpp
    src/core/Performance.cpp
    src/core/Portfolio.cpp
    src/data/HFTDataHandler.cpp
    src/data/HistoricCSVDataHandler.cpp
    src/data/DatabaseDataHandler.cpp   # <--- ADDED
    src/execution/SimulatedExecutionHandler.cpp
    src/risk/RiskManager.cpp
    src/strategy/OrderBookImbalanceStrategy.cpp
    src/strategy/PairTradingStrategy.cpp
    # Add main.cpp if you have one
)

# --- Link Libraries ---
target_link_libraries(backtester PRIVATE
    nlohmann_json::nlohmann_json
    ${PostgreSQL_LIBRARIES}
    PkgConfig::PQXX
    cpr::cpr                  # <--- ADDED cpr library
)

# --- Set Target Include Directories ---
target_include_directories(backtester PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)


cmake_minimum_required(VERSION 3.10)
project(Live_Strategy_Backtester)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Packages ---
# Add pthreads for multi-threading
find_package(Threads REQUIRED)
# Add nlohmann_json (assuming it's in the lib/ directory or installed)
add_subdirectory(lib/nlohmann)

# --- Project Sources ---
# Core components
file(GLOB CORE_SOURCES "src/core/*.cpp")
# Data handlers
file(GLOB DATA_SOURCES "src/data/*.cpp")
# Execution handlers
file(GLOB EXECUTION_SOURCES "src/execution/*.cpp")
# Risk management
file(GLOB RISK_SOURCES "src/risk/*.cpp")
# Strategies
file(GLOB STRATEGY_SOURCES "src/strategy/*.cpp")
# STAGE 6: Analytics
file(GLOB ANALYTICS_SOURCES "src/analytics/*.cpp")


# --- Executable ---
add_executable(backtester src/main.cpp ${CORE_SOURCES} ${DATA_SOURCES} ${EXECUTION_SOURCES} ${RISK_SOURCES} ${STRATEGY_SOURCES} ${ANALYTICS_SOURCES})

# --- Include Directories ---
target_include_directories(backtester PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib"
)

# --- Link Libraries ---
target_link_libraries(backtester PRIVATE Threads::Threads nlohmann_json::nlohmann_json)

# Enable SIMD instructions
target_compile_options(backtester PRIVATE -mavx2 -mfma)


# --- STAGE 8: Unit and Integration Testing with Google Test ---
enable_testing()

find_package(GTest CONFIG)
if (GTest_FOUND)
    message(STATUS "Found GTest, adding tests.")
    
    # Add test files
    file(GLOB TEST_SOURCES "tests/*.cpp")

    add_executable(run_tests ${TEST_SOURCES})
    
    target_include_directories(run_tests PUBLIC 
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib"
    )
    
    # Link tests against the same sources as the main app (or a separate library)
    target_link_libraries(run_tests PRIVATE 
        GTest::gtest GTest::gtest_main Threads::Threads nlohmann_json::nlohmann_json
        ${CORE_SOURCES} ${DATA_SOURCES} ${EXECUTION_SOURCES} ${RISK_SOURCES} ${STRATEGY_SOURCES} ${ANALYTICS_SOURCES}
    )

    include(GoogleTest)
    gtest_discover_tests(run_tests)
else()
    message(WARNING "Google Test not found. Skipping tests.")
endif()

# --- Installation ---
install(TARGETS backtester DESTINATION bin)