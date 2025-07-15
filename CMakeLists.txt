cmake_minimum_required(VERSION 3.16)
project(Live_Strategy_Backtester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Dependencies ---
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

# --- Add Subdirectories and Fetched Content ---
add_subdirectory(lib/nlohmann)

include(FetchContent)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5 # Use a specific tag for stability
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
  websocketpp
  GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
  GIT_TAG 0.8.2
)
FetchContent_MakeAvailable(websocketpp)

FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-28-0
)
FetchContent_MakeAvailable(asio)

# --- Project Sources ---
file(GLOB CORE_SOURCES "src/core/*.cpp")
file(GLOB DATA_SOURCES "src/data/*.cpp")
file(GLOB EXECUTION_SOURCES "src/execution/*.cpp")
file(GLOB RISK_SOURCES "src/risk/*.cpp")
file(GLOB STRATEGY_SOURCES "src/strategy/*.cpp")
file(GLOB ANALYTICS_SOURCES "src/analytics/*.cpp")

# --- Executable ---
add_executable(backtester src/main.cpp 
    ${CORE_SOURCES} 
    ${DATA_SOURCES} 
    ${EXECUTION_SOURCES} 
    ${RISK_SOURCES} 
    ${STRATEGY_SOURCES} 
    ${ANALYTICS_SOURCES}
)

# --- Include Directories ---
target_include_directories(backtester PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib"
    ${PostgreSQL_INCLUDE_DIRS}
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
)

# --- Link Libraries ---
target_link_libraries(backtester PRIVATE 
    Threads::Threads 
    nlohmann_json::nlohmann_json
    ${PostgreSQL_LIBRARIES}
    PkgConfig::PQXX
    cpr::cpr
)

# --- Compiler Options ---
# Enable SIMD instructions if available
target_compile_options(backtester PRIVATE -mavx2 -mfma)

# --- Testing with Google Test ---
enable_testing()

find_package(GTest CONFIG)
if (GTest_FOUND)
    message(STATUS "Found GTest, adding tests.")
    
    file(GLOB TEST_SOURCES "test/*.cpp")

    add_executable(run_tests ${TEST_SOURCES})
    
    target_include_directories(run_tests PUBLIC 
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib"
        ${websocketpp_SOURCE_DIR}
        ${asio_SOURCE_DIR}/asio/include
    )
    
    target_link_libraries(run_tests PRIVATE 
        GTest::gtest GTest::gtest_main 
        Threads::Threads 
        nlohmann_json::nlohmann_json
        # Link test executable against project sources
        ${CORE_SOURCES} 
        ${DATA_SOURCES} 
        ${EXECUTION_SOURCES} 
        ${RISK_SOURCES} 
        ${STRATEGY_SOURCES} 
        ${ANALYTICS_SOURCES}
    )

    include(GoogleTest)
    gtest_discover_tests(run_tests)
else()
    message(WARNING "Google Test not found. Skipping tests.")
endif()

# --- Installation ---
install(TARGETS backtester DESTINATION bin)