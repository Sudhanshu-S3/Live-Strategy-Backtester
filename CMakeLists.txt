cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Live_Strategy_Backtester)

# Add the 'include' directory to the include path
include_directories(include)

# Add the executable, specify its name and its source files
add_executable(backtester src/main.cpp)