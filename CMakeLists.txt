# In CMakeLists.txt

cmake_minimum_required(VERSION 3.14)

project(Live_Strategy_Backtester)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add both 'include' and 'lib' to the include paths
include_directories(include lib)

# Add all our .cpp files to the executable target
add_executable(backtester 
    src/main.cpp
    src/HistoricCSVDataHandler.cpp
    src/SimpleMovingAverageCrossover.cpp
    src/Backtester.cpp
    src/SimulatedExecutionHandler.cpp
    src/Portfolio.cpp
    src/Performance.cpp
)