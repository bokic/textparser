#include <gtest/gtest.h>
#include <textparser.hpp>
#include <cfml_definition.json.h>
#include "tokenparser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

TEST(ParserPerformance, cfml_workspace_benchmark) {
#ifdef __SANITIZE_ADDRESS__
    GTEST_SKIP() << "Skipping benchmark under AddressSanitizer";
#endif
    namespace fs = std::filesystem;
    struct FileSample {
        std::string path;
        std::string content;
    };

    std::vector<FileSample> samples;
    size_t total_bytes = 0;

    for (const auto &entry : fs::recursive_directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".cfm" || ext == ".cfc") {
                std::ifstream f(entry.path(), std::ios::binary);
                if (!f.is_open()) continue;

                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                if (content.empty()) continue;

                total_bytes += content.size();
                samples.push_back({entry.path().string(), std::move(content)});
            }
        }
    }

    ASSERT_FALSE(samples.empty()) << "No CFML (.cfm/.cfc) files found for performance benchmarking.";

    textparser_suppress_errors() = true;

    // Warm-up pass
    for (const auto &sample : samples) {
        TextParser parser(sample.content.c_str(), &cfml_definition);
    }

    // Benchmark iterations
    constexpr int ITERATIONS = 5;
    std::vector<double> iteration_times_ms;
    iteration_times_ms.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        auto start = std::chrono::high_resolution_clock::now();

        for (const auto &sample : samples) {
            TextParser parser(sample.content.c_str(), &cfml_definition);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        iteration_times_ms.push_back(duration_ms);
    }

    textparser_suppress_errors() = false;

    double avg_time_ms = std::accumulate(iteration_times_ms.begin(), iteration_times_ms.end(), 0.0) / ITERATIONS;
    double min_time_ms = *std::min_element(iteration_times_ms.begin(), iteration_times_ms.end());
    double max_time_ms = *std::max_element(iteration_times_ms.begin(), iteration_times_ms.end());

    double mb_parsed = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    double throughput_mb_s = mb_parsed / (avg_time_ms / 1000.0);
    double avg_time_per_file_ms = avg_time_ms / samples.size();

    std::cout << "\n========================================================================\n";
    std::cout << "                   PARSER PERFORMANCE BENCHMARK REPORT                   \n";
    std::cout << "========================================================================\n";
    std::cout << " Target Files          : " << samples.size() << " (.cfm/.cfc files)\n";
    std::cout << " Total Workload        : " << mb_parsed << " MB (" << total_bytes << " bytes)\n";
    std::cout << " Average Total Time    : " << avg_time_ms << " ms\n";
    std::cout << " Min / Max Time        : " << min_time_ms << " ms / " << max_time_ms << " ms\n";
    std::cout << " Latency per File      : " << avg_time_per_file_ms << " ms/file\n";
    std::cout << " Throughput            : " << throughput_mb_s << " MB/s\n";
    std::cout << "========================================================================\n\n";

    EXPECT_GT(throughput_mb_s, 0.0);
}
