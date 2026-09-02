#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree_sitter/api.h>

extern const TSLanguage *tree_sitter_typescript(void);
extern const TSLanguage *tree_sitter_tsx(void);

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s <file.ts|file.tsx> [--outline] [--positions]\n"
            "  (default)  print full S-expression of the parse tree\n"
            "  --outline  print one indented line per named node\n"
            "  --positions include [start_row+1, end_row+1] on each node line\n",
            prog);
}

static int is_tsx_file(const char *path) {
    const char *dot = strrchr(path, '.');
    return dot && strcmp(dot, ".tsx") == 0;
}

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (size > 0 && fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[size] = '\0';
    fclose(f);
    *out_len = (size_t)size;
    return buf;
}

static void dump_sexp(TSNode n) {
    if (ts_node_is_missing(n)) {
        printf("(MISSING %s)", ts_node_type(n));
        return;
    }
    int named = ts_node_is_named(n);
    uint32_t nc = ts_node_child_count(n);
    const char *type = ts_node_type(n);
    if (named) {
        printf("(%s", type);
        for (uint32_t i = 0; i < nc; i++) {
            printf(" ");
            dump_sexp(ts_node_child(n, i));
        }
        printf(")");
    } else if (nc > 0) {
        printf("(%s", type);
        for (uint32_t i = 0; i < nc; i++) {
            printf(" ");
            dump_sexp(ts_node_child(n, i));
        }
        printf(")");
    } else {
        printf("\"%s\"", type);
    }
}

static void print_outline(TSNode node, int depth, int positions) {
    if (depth == 0) return;
    const char *type = ts_node_type(node);
    int named = ts_node_is_named(node);
    if (named || positions) {
        for (int i = 0; i < depth; i++) printf("  ");
        if (named)
            printf("%s", type);
        else
            printf("\"%s\"", type);
        if (ts_node_is_missing(node)) printf(" MISSING");
        if (positions) {
            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);
            printf(" [%u, %u]", start.row + 1, end.row + 1);
        }
        printf("\n");
    }
    uint32_t nc = ts_node_child_count(node);
    for (uint32_t i = 0; i < nc; i++) print_outline(ts_node_child(node, i), depth + 1, positions);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    const char *path = argv[1];
    int outline = 0, positions = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--outline") == 0)
            outline = 1;
        else if (strcmp(argv[i], "--positions") == 0)
            positions = 1;
        else {
            usage(argv[0]);
            return 1;
        }
    }

    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }

    const TSLanguage *lang = is_tsx_file(path) ? tree_sitter_tsx() : tree_sitter_typescript();
    TSParser *parser = ts_parser_new();
    if (!ts_parser_set_language(parser, lang)) {
        fprintf(stderr, "language ABI mismatch\n");
        return 1;
    }
    TSTree *tree = ts_parser_parse_string(parser, NULL, src, (uint32_t)len);
    TSNode root = ts_tree_root_node(tree);

    if (outline)
        print_outline(root, 1, positions);
    else
        dump_sexp(root);
    printf("\n");

    fprintf(stderr, "ROOT_ERROR=%d\n", ts_node_has_error(root));
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    free(src);
    return 0;
}
