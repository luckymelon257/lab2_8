/*
 * Kravchyk Bohdan K-25
 * lab2: variant 8 (replace_if)
 * - C++20
 * - MSVC v143 (Visual Studio 2022)
 * - CMake 3.12
 * - CPU: AMD Ryzen 7 5800U 8-Core 16-Thread
 * - RAM: 16 GB
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <chrono>
#include <random>
#include <thread>
#include <iomanip>
#include <limits>
#include <numeric>

using DataValue = int;
using DataContainer = std::vector<DataValue>;

const DataValue THRESHOLD = 50;
const DataValue NEW_VALUE = 0;

bool predicate(DataValue n) {
    return n > THRESHOLD;
}

DataContainer generate_data(size_t size) {
    std::cout << "Generating " << size << " random elements... ";
    DataContainer data(size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dist(1, 100);

    std::generate(data.begin(), data.end(), [&]() { return dist(gen); });

    std::cout << "Done.\n";

    return data;
}

template <typename Func>
double measure_time_ms(Func func) {
    auto start = std::chrono::high_resolution_clock::now();

    func();

    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double, std::milli>(end - start).count();
}

void custom_parallel_replace(DataContainer& data, int K) {
    if (K <= 0) return;

    if (K == 1) {
        std::replace_if(data.begin(), data.end(), predicate, NEW_VALUE);

        return;
    }

    std::vector<std::jthread> threads;
    threads.reserve(K);

    size_t size = data.size();
    size_t chunk_size = size / K;

    auto worker = [](auto start_itr, auto end_itr) {
        std::replace_if(start_itr, end_itr, predicate, NEW_VALUE);
    };

    for (int i = 0; i < K; ++i) {
        auto start_itr = data.begin() + i * chunk_size;
        auto end_itr = (i == K - 1) ? data.end() : (start_itr + chunk_size);

        threads.emplace_back(worker, start_itr, end_itr);
    }
}

void run_experiment(size_t data_size) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "EXPERIMENT DATA SIZE: " << data_size << "\n";
    std::cout << std::string(60, '=') << "\n";

    DataContainer source_data = generate_data(data_size);
    DataContainer working_data;

    working_data = source_data;
    double seq_time = measure_time_ms([&]() {
        std::replace_if(working_data.begin(), working_data.end(), predicate, NEW_VALUE);
        });

    std::cout << std::left << std::setw(40) << "std::replace_if (Sequential):"
        << std::fixed << std::setprecision(2) << seq_time << " ms\n";


    working_data = source_data;
    double par_time = measure_time_ms([&]() {
        std::replace_if(std::execution::par, working_data.begin(), working_data.end(), predicate, NEW_VALUE);
        });
    std::cout << std::left << std::setw(40) << "std::execution::par:" << par_time << " ms\n";

    working_data = source_data;
    double par_unseq_time = measure_time_ms([&]() {
        std::replace_if(std::execution::par_unseq, working_data.begin(), working_data.end(), predicate, NEW_VALUE);
        });
    std::cout << std::left << std::setw(40) << "std::execution::par_unseq:" << par_unseq_time << " ms\n";

    std::cout << "\n--- Custom Parallel Algorithm Analysis ---\n";
    unsigned int hw_threads = std::thread::hardware_concurrency();


    std::cout << "Hardware Threads available: " << hw_threads << "\n\n";

    std::cout << "| " << std::setw(10) << "K (Threads)"
        << " | " << std::setw(15) << "Time (ms)"
        << " | " << std::setw(15) << "Speedup (vs Seq)" << " |\n";

    std::cout << "|" << std::string(12, '-') << "|" << std::string(17, '-') << "|" << std::string(18, '-') << "|\n";

    int best_k = 1;
    double min_custom_time = std::numeric_limits<double>::max();

    std::vector<int> k_values;
    for (int k = 1; k <= static_cast<int>(hw_threads) + 4; ++k) k_values.push_back(k);

    k_values.push_back(hw_threads * 2);
    k_values.push_back(hw_threads * 4);

    for (int k : k_values) {
        working_data = source_data;

        double current_time = measure_time_ms([&]() {
            custom_parallel_replace(working_data, k);
            });

        if (current_time < min_custom_time) {
            min_custom_time = current_time;
            best_k = k;
        }

        double speedup = seq_time / current_time;

        std::cout << "| " << std::setw(10) << k
            << " | " << std::setw(15) << current_time
            << " | " << std::setw(14) << speedup << "x" << " |\n";
    }

    std::cout << "\n[RESULT SUMMARY]\n";
    std::cout << "Best K: " << best_k << "\n";
    std::cout << "Ratio (Best K / HW Threads): " << (double)best_k / hw_threads << "\n";
    std::cout << "Max Speedup: " << (seq_time / min_custom_time) << "x\n";
}

int main() {
    run_experiment(20'000'000);
    run_experiment(50'000'000);
    run_experiment(100'000'000);

    return 0;
}