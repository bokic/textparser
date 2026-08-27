#include <benchmark/benchmark.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <c_definition.json.h>
#include <tree_sitter/api.h>

extern "C" const TSLanguage *tree_sitter_c(void);

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

struct FileEntry {
    std::string path;
    size_t      bytes;
};

static std::vector<FileEntry> g_c_files;
static std::vector<FileEntry> g_h_files;

static void collect_file_paths(const std::string &root,
                               const std::string &ext,
                               std::vector<FileEntry> &out)
{
    namespace fs = std::filesystem;
    if (!fs::exists(root)) return;
    for (auto &entry : fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ext) continue;
        out.push_back({entry.path().string(), entry.file_size()});
    }
}

// ── Benchmark: parse .c files one at a time (load -> parse -> ast -> free) ───

static void BM_TextParser_ParseC(benchmark::State &state)
{
    if (g_c_files.empty()) {
        state.SkipWithError("No .c files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_c_files) {
            textparser::Parser parser;
            if (parser.openfile(fe.path.c_str(), c_definition.default_text_encoding, TEXTPARSER_BOM_ALL) == 0) {
                parser.parse(&c_definition);
                // Verify AST root item
                benchmark::DoNotOptimize(parser.get_first_token());
            }
            total_bytes += fe.bytes;
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

// ── Benchmark: parse .h files one at a time (load -> parse -> ast -> free) ───

static void BM_TextParser_ParseH(benchmark::State &state)
{
    if (g_h_files.empty()) {
        state.SkipWithError("No .h files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_h_files) {
            textparser::Parser parser;
            if (parser.openfile(fe.path.c_str(), c_definition.default_text_encoding, TEXTPARSER_BOM_ALL) == 0) {
                parser.parse(&c_definition);
                // Verify AST root item
                benchmark::DoNotOptimize(parser.get_first_token());
            }
            total_bytes += fe.bytes;
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

// ── Helper to read an entire file into memory ─────────────────────────────────

static std::string read_file_content(const std::string &path)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return "";
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::string buffer(size, '\0');
    if (ifs.read(&buffer[0], size)) {
        return buffer;
    }
    return "";
}

// ── TextParser JSON benchmark: parse .c files using PCRE2 (JSON-loaded definition) ──

static textparser_language_definition *g_c_json_def = nullptr;

static void BM_TextParser_ParseC_JSON(benchmark::State &state)
{
    if (g_c_files.empty()) {
        state.SkipWithError("No .c files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_c_files) {
            textparser::Parser parser;
            if (parser.openfile(fe.path.c_str(), g_c_json_def->default_text_encoding, TEXTPARSER_BOM_ALL) == 0) {
                parser.parse(g_c_json_def);
                benchmark::DoNotOptimize(parser.get_first_token());
            }
            total_bytes += fe.bytes;
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

// ── TextParser JSON benchmark: parse .h files using PCRE2 (JSON-loaded definition) ──

static textparser_language_definition *g_h_json_def = nullptr;

static void BM_TextParser_ParseH_JSON(benchmark::State &state)
{
    if (g_h_files.empty()) {
        state.SkipWithError("No .h files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_h_files) {
            textparser::Parser parser;
            if (parser.openfile(fe.path.c_str(), g_h_json_def->default_text_encoding, TEXTPARSER_BOM_ALL) == 0) {
                parser.parse(g_h_json_def);
                benchmark::DoNotOptimize(parser.get_first_token());
            }
            total_bytes += fe.bytes;
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

// ── Tree-sitter Benchmark: parse .c files one at a time (load -> parse -> ast -> free) ──

static void BM_TreeSitter_ParseC(benchmark::State &state)
{
    if (g_c_files.empty()) {
        state.SkipWithError("No .c files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_c_files) {
            std::string content = read_file_content(fe.path);
            TSParser *parser = ts_parser_new();
            ts_parser_set_language(parser, tree_sitter_c());
            TSTree *tree = ts_parser_parse_string(parser, nullptr, content.c_str(), content.size());
            TSNode root = ts_tree_root_node(tree);
            benchmark::DoNotOptimize(root);
            ts_tree_delete(tree);
            ts_parser_delete(parser);
            total_bytes += fe.bytes;
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

// ── Tree-sitter Benchmark: parse .h files one at a time (load -> parse -> ast -> free) ──

static void BM_TreeSitter_ParseH(benchmark::State &state)
{
    if (g_h_files.empty()) {
        state.SkipWithError("No .h files found in SQLITE_SRC_DIR");
        return;
    }

    long dirty_before = read_vm_dirty_kb();
    size_t total_bytes = 0;

    for (auto _ : state) {
        for (const auto &fe : g_h_files) {
            std::string content = read_file_content(fe.path);
            TSParser *parser = ts_parser_new();
            ts_parser_set_language(parser, tree_sitter_c());
            TSTree *tree = ts_parser_parse_string(parser, nullptr, content.c_str(), content.size());
            TSNode root = ts_tree_root_node(tree);
            benchmark::DoNotOptimize(root);
            ts_tree_delete(tree);
            ts_parser_delete(parser);
            total_bytes += fe.bytes;
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

BENCHMARK(BM_TextParser_ParseC)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_TextParser_ParseC_JSON)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_TreeSitter_ParseC)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_TextParser_ParseH)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_TextParser_ParseH_JSON)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

BENCHMARK(BM_TreeSitter_ParseH)
    ->Unit(benchmark::kMillisecond)
    ->MinWarmUpTime(1.0)
    ->Iterations(3);

int main(int argc, char **argv)
{
    const std::string sqlite_src = SQLITE_SRC_DIR;

    collect_file_paths(sqlite_src, ".c", g_c_files);
    collect_file_paths(sqlite_src, ".h", g_h_files);

    fprintf(stdout, "SQLite source: %s\n", sqlite_src.c_str());
    fprintf(stdout, "  .c files: %zu\n", g_c_files.size());
    fprintf(stdout, "  .h files: %zu\n", g_h_files.size());

    size_t c_bytes = 0, h_bytes = 0;
    for (const auto &f : g_c_files) c_bytes += f.bytes;
    for (const auto &f : g_h_files) h_bytes += f.bytes;
    fprintf(stdout, "  .c total: %.2f MB\n", c_bytes / 1024.0 / 1024.0);
    fprintf(stdout, "  .h total: %.2f MB\n", h_bytes / 1024.0 / 1024.0);
    fflush(stdout);

    int json_err = textparser_json_load_language_definition_from_json_file(
        C_DEF_JSON_PATH, &g_c_json_def);
    if (json_err != TEXTPARSER_JSON_NO_ERROR) {
        fprintf(stderr, "Failed to load C definition from JSON: %s\n",
                textparser_json_strerror(json_err));
        return 1;
    }
    g_h_json_def = g_c_json_def;
    fprintf(stdout, "Loaded C definition from JSON: %s\n", C_DEF_JSON_PATH);
    fflush(stdout);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    textparser_free_language_definition(g_c_json_def);
    return 0;
}
