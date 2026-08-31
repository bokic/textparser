#include <textparser.h>
#include <os.h>
#include <ada_definition.json.h>
#include <asm_definition.json.h>
#include <bash_definition.json.h>
#include <c3_definition.json.h>
#include <c_definition.json.h>
#include <cfml_definition.json.h>
#include <cpp_definition.json.h>
#include <csharp_definition.json.h>
#include <css_definition.json.h>
#include <fortran_definition.json.h>
#include <go_definition.json.h>
#include <html_definition.json.h>
#include <jai_definition.json.h>
#include <java_definition.json.h>
#include <javascript_definition.json.h>
#include <json_definition.json.h>
#include <matlab_definition.json.h>
#include <md_definition.json.h>
#include <pascal_definition.json.h>
#include <perl_definition.json.h>
#include <php_definition.json.h>
#include <python_definition.json.h>
#include <r_definition.json.h>
#include <rust_definition.json.h>
#include <scratch_definition.json.h>
#include <sql_definition.json.h>
#include <swift_definition.json.h>
#include <typescript_definition.json.h>
#include <vb_definition.json.h>
#include <zig_definition.json.h>

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>


#define KB * 1024
#define MB * 1024 KB
#define GB * 1024 MB
#define MAX_PARSE_SIZE (4 MB)

const textparser_language_definition *definitions[] = {
    &ada_definition,
    &asm_definition,
    &bash_definition,
    &c3_definition,
    &c_definition,
    &cfml_definition,
    &cpp_definition,
    &csharp_definition,
    &css_definition,
    &fortran_definition,
    &go_definition,
    &html_definition,
    &jai_definition,
    &java_definition,
    &javascript_definition,
    &json_definition,
    &matlab_definition,
    &md_definition,
    &pascal_definition,
    &perl_definition,
    &php_definition,
    &python_definition,
    &r_definition,
    &rust_definition,
    &scratch_definition,
    &sql_definition,
    &swift_definition,
    &typescript_definition,
    &vb_definition,
    &zig_definition,
    nullptr
};

static const textparser_language_definition *get_language_definition_by_filename(const char *filename)
{
    const textparser_language_definition *definition = nullptr;
    const char *definition_ext = nullptr;
    int def_cnt = 0;

    const char *filename_ext = strrchr(filename, '.');
    if (filename_ext == nullptr)
        return nullptr;

    filename_ext++;

    while(definitions[def_cnt] != nullptr)
    {
        int ext_cnt = 0;

        definition = definitions[def_cnt];

        while(definition->default_file_extensions[ext_cnt] != nullptr)
        {
            definition_ext = definition->default_file_extensions[ext_cnt];

            bool match = false;
            if (definition->case_sensitivity) {
                match = (strcmp(definition_ext, filename_ext) == 0);
            } else {
#ifdef _WIN32
                match = (_stricmp(definition_ext, filename_ext) == 0);
#else
                match = (strcasecmp(definition_ext, filename_ext) == 0);
#endif
            }
            if (match)
                return definition;

            ext_cnt++;
        }

        def_cnt++;
    }

    return nullptr;
}

static void print_element(const char *text, size_t len, const char *text_background, const char *text_color, const char *text_flags)
{
    static const char * const reset_ansi = "\33[0m";
    bool clear_format = false;

    if (strlen(text_background)) { os_write_to_terminal(text_background, strlen(text_background)); clear_format = true; }
    if (strlen(text_color))      { os_write_to_terminal(text_color, strlen(text_color));           clear_format = true; }
    if (strlen(text_flags))      { os_write_to_terminal(text_flags, strlen(text_flags));           clear_format = true; }
    os_write_to_terminal(text, len);
    if (clear_format)    os_write_to_terminal(reset_ansi, strlen(reset_ansi));
}

static void print_token_range(const char *text, const textparser_token_range *range)
{
    char ansi_format_background[64] = {0, };
    char ansi_format_text_color[64] = {0, };
    char ansi_format_flags[64] = {0, };

    if (range->text_background != TEXTPARSER_NOCOLOR) {
        snprintf(ansi_format_background, sizeof(ansi_format_background),
            "\33[48;2;%u;%u;%um",
            (range->text_background >>  0) & 0xff,
            (range->text_background >>  8) & 0xff,
            (range->text_background >> 16) & 0xff
        );
    }

    if (range->text_color != TEXTPARSER_NOCOLOR) {
        snprintf(ansi_format_text_color, sizeof(ansi_format_text_color),
            "\33[38;2;%u;%u;%um",
            (range->text_color >> 16) & 0xff,
            (range->text_color >>  8) & 0xff,
            (range->text_color >>  0) & 0xff
        );
    }

    if (range->text_flags & 0x01) strcat(ansi_format_flags, "\33[1m");
    if (range->text_flags & 0x02) strcat(ansi_format_flags, "\33[2m");
    if (range->text_flags & 0x04) strcat(ansi_format_flags, "\33[3m");
    if (range->text_flags & 0x08) strcat(ansi_format_flags, "\33[4m");

    print_element(text + range->start_pos, range->length, ansi_format_background, ansi_format_text_color, ansi_format_flags);
}

int main(int argc, const char *argv[])
{
    const textparser_language_definition *language_def = nullptr;
    bool should_end_with_newline = false;
    const char *filename = nullptr;

    textparser_defer(handle);
    int res = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage ccat <text file>|--version.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--version") == 0)
    {
        const char *library_version = textparser_version();
        if (strcmp(TEXTPARSER_BINARY_VERSION, library_version) == 0)
            printf("ccat version %s\n", TEXTPARSER_BINARY_VERSION);
        else
            printf("ccat version %s(executable), %s(library)\n", TEXTPARSER_BINARY_VERSION, library_version);
        return EXIT_SUCCESS;
    }

    filename = argv[1];

    language_def = get_language_definition_by_filename(filename);
    if (language_def == nullptr) {
        fprintf(stderr, "Unsupported file extension for file '%s'\n", filename);
        return EXIT_FAILURE;
    }

    res = textparser_openfile(filename, language_def->default_text_encoding, language_def->supported_bom, &handle);
    if (res) {
        fprintf(stderr, "Error opening file '%s': %s (code %d)\n", filename, textparser_strerror(res), res);
        return EXIT_FAILURE;
    }

    res = textparser_parse(handle, language_def);
    if (res) {
        const char *detail = textparser_parse_error(handle);
        size_t error_pos = textparser_parse_error_position(handle);
        if (detail) {
            fprintf(stderr, "Error parsing file '%s' at offset %zu: %s (code %d)\n", filename, error_pos, detail, res);
        } else {
            fprintf(stderr, "Error parsing file '%s': %s (code %d)\n", filename, textparser_strerror(res), res);
        }
        return EXIT_FAILURE;
    }

    textparser_token_item *root = textparser_get_first_token(handle);
    textparser_post_process(&root, language_def);

    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);

    if ((text_size > 0)&&(text[text_size - 1] != '\n')) {
        should_end_with_newline = true;
    }

    size_t token_count = 0;
    textparser_export_tokens(handle, nullptr, 0, &token_count);

    if (token_count > 0) {
        textparser_token_range *ranges = malloc(sizeof(textparser_token_range) * token_count);
        if (ranges && textparser_export_tokens(handle, ranges, token_count, &token_count) == 0) {
            size_t pos = 0;
            for (size_t i = 0; i < token_count; i++) {
                if (ranges[i].start_pos > pos) {
                    os_write_to_terminal(text + pos, ranges[i].start_pos - pos);
                }
                print_token_range(text, &ranges[i]);
                pos = ranges[i].start_pos + ranges[i].length;
            }
            if (text_size > pos) {
                os_write_to_terminal(text + pos, text_size - pos);
            }
        }
        free(ranges);
    } else {
        os_write_to_terminal(text, text_size);
    }

    if (should_end_with_newline) {
        os_write_to_terminal("\n", 1);
    }

    return EXIT_SUCCESS;
}
