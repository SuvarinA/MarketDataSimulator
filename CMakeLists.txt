cmake_minimum_required(VERSION 3.10) # Adjust version as needed
project(MarketDataSimulator LANGUAGES CXX)

add_executable(MarketDataSimulator main.cpp marketData.cpp)
target_precompile_headers(MarketDataSimulator PRIVATE <iostream> <string> marketData.h)
