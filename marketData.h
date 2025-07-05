#ifndef SIMPLE_MARKET_DATA_H
#define SIMPLE_MARKET_DATA_H

#include <string>     // For std::string
#include <chrono>     // For std::chrono::system_clock::time_point
#include <random>     // For std::mt19937, std::uniform_real_distribution, std::uniform_int_distribution

// Structure to represent a single market data tick
struct MarketDataTick {
    std::chrono::system_clock::time_point timestamp;
    std::string symbol;
    double price;
    long volume;

    // Declaration of the helper function
    std::string getFormattedTimestamp() const;
};

// Class to generate simple market data ticks
class MarketDataGenerator {
public:
    // Constructor declaration
    MarketDataGenerator(std::string symbol, double initialPrice, long initialVolume);

    // Public getter for the symbol
    const std::string& getSymbol() const;

    // Method to generate a single market data tick
    MarketDataTick generateTick();

private:
    std::string symbol_;
    double currentPrice_;
    long currentVolume_;

    // Random number generators and distributions
    std::mt19937 priceGen_;
    std::mt19937 volumeGen_;
    std::uniform_real_distribution<> priceDist_;
    std::uniform_int_distribution<> volumeDist_;
};

#endif // SIMPLE_MARKET_DATA_H
