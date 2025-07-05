#include "marketData.h" // Include the header file for declarations
#include <iostream>   // For cout (e.g., if you add debug prints inside methods)
#include <iomanip>    // For fixed, setprecision, put_time
#include <sstream>    // For ostringstream
#include <ctime>      // For localtime

using namespace std;

// --- MarketDataTick Method Implementation ---

string MarketDataTick::getFormattedTimestamp() const {
    time_t tt = chrono::system_clock::to_time_t(timestamp);
    tm tm = {};
#if defined(_MSC_VER)
    localtime_s(&tm, &tt); // Use the safe version on MSVC
#else
    tm = *localtime(&tt); // For non-MSVC compilers
#endif

    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto ms = chrono::duration_cast<chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    oss << "." << setfill('0') << setw(3) << ms.count();
    return oss.str();
}

// --- MarketDataGenerator Class Implementations ---

MarketDataGenerator::MarketDataGenerator(string symbol, double initialPrice, long initialVolume)
    : symbol_(move(symbol)),
      currentPrice_(initialPrice),
      currentVolume_(initialVolume),
      priceGen_(random_device()()),
      volumeGen_(random_device()()),
      priceDist_(-0.5, 0.5),
      volumeDist_(1, 100)
{}

const string& MarketDataGenerator::getSymbol() const {
    return symbol_;
}

MarketDataTick MarketDataGenerator::generateTick() {
    MarketDataTick tick;
    tick.timestamp = chrono::system_clock::now();
    tick.symbol = symbol_;

    currentPrice_ += priceDist_(priceGen_) * 0.1;
    if (currentPrice_ < 0.01) {
        currentPrice_ = 0.01;
    }

    currentVolume_ += volumeDist_(volumeGen_);
    if (currentVolume_ < 1) {
        currentVolume_ = 1;
    }

    tick.price = currentPrice_;
    tick.volume = currentVolume_;

    return tick;
}
