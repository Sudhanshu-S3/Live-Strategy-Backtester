# In CMakeLists.txt

cmake_minimum_required(VERSION 3.14)

project(Live_Strategy_Backtester)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add 'include' and 'lib' to the include paths
include_directories(
    include
    lib
    include/core
    include/data
    include/execution
    include/strategy
)

# Add all our .cpp files to the executable target
add_executable(backtester
    src/main.cpp
    src/data/HistoricCSVDataHandler.cpp
    src/data/HFTDataHandler.cpp
    src/strategy/PairTradingStrategy.cpp
    src/core/Backtester.cpp
    src/execution/SimulatedExecutionHandler.cpp
    src/core/Portfolio.cpp
    src/core/Performance.cpp
)