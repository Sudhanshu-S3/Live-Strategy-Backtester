cmake_minimum_required(VERSION 3.14) # Upped to 3.14 for better C++17 support

# Set the project name
project(Live_Strategy_Backtester)

# Set C++ standard to 17 to use features like std::optional and if-init statements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the 'include' directory to the include path
include_directories(include)

# Add the executable, specify its name and ALL its source files
add_executable(backtester
    src/main.cpp
    src/HistoricCSVDataHandler.cpp
    src/Backtester.cpp
    src/SimpleMovingAverageCrossover.cpp
    src/SimulatedExecutionHandler.cpp
)

# Link libraries if any (none in this case, but good practice to have)