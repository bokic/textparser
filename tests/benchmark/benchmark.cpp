#include <textparser.hpp>
#include <cfml_definition.json.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <string>

static const std::vector<std::string> SKIP_DIRS = {"build", "bin", ".git", ".cache", "build_asan", "build_release"};

static bool should_skip(const std::filesystem::path &p)
{
    for (const auto &skip : SKIP_DIRS) {
        if (p.filename() == skip) return true;
    }
    return false;
}

static void collect_cfml_files(const std::filesystem::path &root,
                                std::vector<std::string> &paths)
{
    namespace fs = std::filesystem;
    for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it)
    {
        if (it->is_directory() && should_skip(it->path())) {
            it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file()) continue;
        const std::string ext = it->path().extension().string();
        if (ext == ".cfm" || ext == ".cfc") {
            paths.push_back(it->path().string());
        }
    }
}

static bool parse_file(const std::string &path)
{
    textparser::Parser parser;
    if (parser.openfile(path.c_str(), cfml_definition.default_text_encoding, cfml_definition.supported_bom) != 0) {
        return false;
    }
    return parser.parse(&cfml_definition) == 0;
}

int main(int argc, char **argv)
{
    namespace fs = std::filesystem;
    fs::path search_root = argc > 1 ? fs::path(argv[1]) : fs::current_path();

    std::vector<std::string> paths;
    collect_cfml_files(search_root, paths);

    if (paths.empty()) {
        std::cerr << "Error: No CFML (.cfm/.cfc) files found under: " << search_root << "\n";
        std::cerr << "Usage: benchmark [search_directory]\n";
        return 1;
    }

    std::cout << "Found " << paths.size() << " files. Running warm-up...\n" << std::flush;

    // Warm-up pass
    for (const auto &path : paths) {
        parse_file(path);
    }

    std::cout << "Warm-up done. Running " << 5 << " benchmark iterations...\n" << std::flush;

    // Benchmark iterations
    constexpr int ITERATIONS = 5;
    std::vector<double> iteration_times_ms;
    iteration_times_ms.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto &path : paths) {
            parse_file(path);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        iteration_times_ms.push_back(ms);
        std::cout << "  iter " << (i + 1) << ": " << ms << " ms\n" << std::flush;
    }

    double avg_time_ms = std::accumulate(iteration_times_ms.begin(), iteration_times_ms.end(), 0.0) / ITERATIONS;
    double min_time_ms = *std::min_element(iteration_times_ms.begin(), iteration_times_ms.end());
    double max_time_ms = *std::max_element(iteration_times_ms.begin(), iteration_times_ms.end());

    // Compute total bytes from file sizes
    size_t total_bytes = 0;
    for (const auto &path : paths) {
        total_bytes += fs::file_size(path);
    }

    double mb_parsed = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    double throughput_mb_s = mb_parsed / (avg_time_ms / 1000.0);
    double avg_time_per_file_ms = avg_time_ms / static_cast<double>(paths.size());

    std::cout << "\n========================================================================\n";
    std::cout << "                   PARSER PERFORMANCE BENCHMARK REPORT                   \n";
    std::cout << "========================================================================\n";
    std::cout << " Search Root           : " << search_root << "\n";
    std::cout << " Target Files          : " << paths.size() << " (.cfm/.cfc files)\n";
    std::cout << " Total Workload        : " << mb_parsed << " MB (" << total_bytes << " bytes)\n";
    std::cout << " Average Total Time    : " << avg_time_ms << " ms\n";
    std::cout << " Min / Max Time        : " << min_time_ms << " ms / " << max_time_ms << " ms\n";
    std::cout << " Latency per File      : " << avg_time_per_file_ms << " ms/file\n";
    std::cout << " Throughput            : " << throughput_mb_s << " MB/s\n";
    std::cout << "========================================================================\n\n";

    return 0;
}
