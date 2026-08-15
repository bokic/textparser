#include <benchmark/benchmark.h>
#include <textparser.hpp>
#include <c_definition.json.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// ── Dirty page helpers (Linux /proc/self/status) ──────────────────────────────

static long read_vm_dirty_kb()
{
#if defined(__linux__)
    // RssAnon approximates private dirty pages for this process
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long kb = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "RssAnon:", 8) == 0) {
            sscanf(line + 8, " %ld", &kb); // NOLINT
            break;
        }
    }
    fclose(f);
    return kb;
#else
    return -1;
#endif
}

// ── File collection ───────────────────────────────────────────────────────────

struct SourceFile {
    std::string path;
    std::string content;
    size_t      bytes;
};

static std::vector<SourceFile> g_c_files;
static std::vector<SourceFile> g_h_files;

static void collect_files(const std::string &root,
                           const std::string &ext,
                           std::vector<SourceFile> &out)
{
    namespace fs = std::filesystem;
    if (!fs::exists(root)) return;
    for (auto &entry : fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ext) continue;
        std::ifstream f(entry.path(), std::ios::binary);
        if (!f.is_open()) continue;
        std::string content((std::istreambuf_iterator<char>(f)), {});
        if (content.empty()) continue;
        out.push_back({entry.path().string(), std::move(content), entry.file_size()});
    }
}

// ── Benchmark: parse all .c files ────────────────────────────────────────────

static void BM_ParseC(benchmark::State &state)
{
    if (g_c_files.empty()) {
        state.SkipWithError("No .c files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &sf : g_c_files) {
            textparser::Parser parser;
            parser.openmem(sf.content.c_str(),
                           static_cast<int>(sf.content.size()),
                           c_definition.default_text_encoding);
            parser.parse(&c_definition);
            total_bytes += sf.bytes;
        }
    }

    long dirty_after = read_vm_dirty_kb();

    state.SetBytesProcessed(static_cast<int64_t>(total_bytes));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(g_c_files.size()));

    if (dirty_before >= 0 && dirty_after >= 0) {
        state.counters["DirtyPages_KB"]  = benchmark::Counter(
            static_cast<double>(dirty_after - dirty_before));
        state.counters["RSS_KB_after"]   = benchmark::Counter(
            static_cast<double>(dirty_after));
    }
    state.counters["Files"]              = benchmark::Counter(
        static_cast<double>(g_c_files.size()));
}

// ── Benchmark: parse all .h files ────────────────────────────────────────────

static void BM_ParseH(benchmark::State &state)
{
    if (g_h_files.empty()) {
        state.SkipWithError("No .h files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &sf : g_h_files) {
            textparser::Parser parser;
            parser.openmem(sf.content.c_str(),
                           static_cast<int>(sf.content.size()),
                           c_definition.default_text_encoding);
            parser.parse(&c_definition);
            total_bytes += sf.bytes;
        }
    }

    long dirty_after = read_vm_dirty_kb();

    state.SetBytesProcessed(static_cast<int64_t>(total_bytes));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(g_h_files.size()));

    if (dirty_before >= 0 && dirty_after >= 0) {
        state.counters["DirtyPages_KB"]  = benchmark::Counter(
            static_cast<double>(dirty_after - dirty_before));
        state.counters["RSS_KB_after"]   = benchmark::Counter(
            static_cast<double>(dirty_after));
    }
    state.counters["Files"]              = benchmark::Counter(
        static_cast<double>(g_h_files.size()));
}

// ── Registration & main ───────────────────────────────────────────────────────

BENCHMARK(BM_ParseC)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_ParseH)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

// Custom main so we can pre-load files before benchmarks run
int main(int argc, char **argv)
{
    // Pre-load SQLite source files into memory once
    const std::string sqlite_src = SQLITE_SRC_DIR;

    collect_files(sqlite_src, ".c", g_c_files);
    collect_files(sqlite_src, ".h", g_h_files);

    fprintf(stdout, "SQLite source: %s\n", sqlite_src.c_str());
    fprintf(stdout, "  .c files: %zu\n", g_c_files.size());
    fprintf(stdout, "  .h files: %zu\n", g_h_files.size());

    size_t c_bytes = 0, h_bytes = 0;
    for (const auto &f : g_c_files) c_bytes += f.bytes;
    for (const auto &f : g_h_files) h_bytes += f.bytes;
    fprintf(stdout, "  .c total: %.2f MB\n", c_bytes / 1024.0 / 1024.0);
    fprintf(stdout, "  .h total: %.2f MB\n", h_bytes / 1024.0 / 1024.0);
    fflush(stdout);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
