#include <queue>      // For queue
#include <mutex>      // For mutex
#include <condition_variable> // For condition_variable
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>      // For fixed and setprecision
#include <sstream>      // For ostringstream
#include <fstream>      // For ofstream (to write to file)
#include "marketData.h" // Include the header file for declarations

using namespace std;

// --- Thread-Safe Queue for MarketDataTick ---
// This queue will allow the main thread (producer) to push ticks
// and the writer thread (consumer) to pop ticks safely.
template <typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        lock_guard<mutex> lock(mtx_); // Acquire lock
        queue_.push(move(value));         // Add item to queue
        cv_.notify_one();                      // Notify one waiting thread
    }

    // Attempts to pop an item without blocking. Returns true if successful, false otherwise.
    bool try_pop(T& value) {
        lock_guard<mutex> lock(mtx_);
        if (queue_.empty()) {
            return false;
        }
        value = move(queue_.front());
        queue_.pop();
        return true;
    }

    // Pops an item, blocking if the queue is empty until an item is available or stop is signaled.
    void wait_and_pop(T& value) {
        unique_lock<mutex> lock(mtx_);
        // Wait until queue is not empty OR stop signal is received
        cv_.wait(lock, [this] { return !queue_.empty() || stop_requested_; });

        if (stop_requested_ && queue_.empty()) {
            // If stop was requested and queue is empty, we are done
            // Re-notify to ensure other waiting threads also wake up and exit if needed
            cv_.notify_all();
            throw runtime_error("ThreadSafeQueue stopped."); // Or handle more gracefully
        }

        value = move(queue_.front());
        queue_.pop();
    }

    // Signals the queue to stop, causing waiting consumers to wake up and exit.
    void stop() {
        lock_guard<mutex> lock(mtx_);
        stop_requested_ = true;
        cv_.notify_all(); // Notify all waiting threads that stop has been requested
    }

private:
    queue<T> queue_;
    mutex mtx_;
    condition_variable cv_;
    bool stop_requested_ = false; // Flag to signal threads to stop
};

// --- Function for the CSV Writer Thread ---
void csvWriterThread(ThreadSafeQueue<MarketDataTick>& tickQueue, const string& filename) {
    ofstream outputFile(filename, ios::out | ios::trunc);

    if (!outputFile.is_open()) {
        cerr << "Error: CSV Writer Thread could not open file " << filename << " for writing." << endl;
        return;
    }

    // Write the CSV header row
    outputFile << "Timestamp,Symbol,Price,Volume\n";

    MarketDataTick tick;
    try {
        while (true) {
            tickQueue.wait_and_pop(tick); // Blocks until a tick is available or stop is requested

            // Write the tick data
            outputFile << tick.getFormattedTimestamp() << ","
                       << tick.symbol << ","
                       << fixed << setprecision(2) << tick.price << ","
                       << tick.volume << "\n";
            outputFile.flush(); // Optional: flush buffer to disk more frequently. Good for debugging/recovery.
        }
    } catch (const runtime_error& e) {
        // Expected exception when stop is requested and queue is empty
        cout << "[CSV Writer] Thread stopped: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "[CSV Writer] An unexpected error occurred: " << e.what() << endl;
    }

    outputFile.close();
    cout << "[CSV Writer] File " << filename << " closed." << endl;
}


// --- Main Application Logic ---
int main() {
    // --- Setup Multiple MarketDataGenerators ---
    vector<MarketDataGenerator> generators;
    generators.emplace_back("GOOG", 150.00, 1000);
    generators.emplace_back("AAPL", 175.50, 1200);
    generators.emplace_back("MSFT", 420.10, 800);
    generators.emplace_back("AMZN", 180.75, 1500);
    generators.emplace_back("TSLA", 200.00, 900);

    // --- Setup Thread-Safe Queue and Writer Thread ---
    ThreadSafeQueue<MarketDataTick> tickQueue;
    const string filename = "multi_symbol_threaded_market_data_output2.csv";

    // Create and start the writer thread
    thread writerThread(csvWriterThread, ref(tickQueue), filename);

    cout << "Generating market data for multiple symbols and queuing for writing to "
              << filename << ". Press Ctrl+C to stop." << endl;
    cout << "---------------------------------------------------------" << endl;
    // Console header for immediate feedback
    cout << left << setw(25) << "Timestamp"
              << left << setw(10) << "Symbol"
              << left << setw(15) << "Price"
              << left << "Volume" << endl;
    cout << "---------------------------------------------------------" << endl;

    // --- Main Simulation Loop (Producer) ---
    const int num_simulation_steps = 50; // More steps to see data accumulate
    const chrono::milliseconds time_step_delay(100); // Shorter delay

    for (int step = 0; step < num_simulation_steps; ++step) {
        for (auto& generator : generators) {
            MarketDataTick tick = generator.generateTick();

            // Print to console (for real-time observation)
            cout << fixed << setprecision(2)
                      << left << setw(25) << tick.getFormattedTimestamp()
                      << left << setw(10) << tick.symbol
                      << left << setw(15) << tick.price
                      << left << tick.volume << endl;

            // Push the tick to the thread-safe queue for the writer thread
            tickQueue.push(tick);
        }
        this_thread::sleep_for(time_step_delay);
    }

    // --- Shutdown Process ---
    cout << "\n---------------------------------------------------------" << endl;
    cout << "Simulation finished. Signaling writer thread to stop..." << endl;

    tickQueue.stop(); // Signal the writer thread to stop processing new items

    writerThread.join(); // Wait for the writer thread to finish its work and terminate

    cout << "All data written and threads joined. Application exiting." << endl;

    return 0;
}
