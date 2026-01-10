#include <textparser.h>
#include <cfml_definition.json.h>
#include <json_definition.json.h>
//#include <php_definition.json.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>


#define KB * 1024
#define MB * 1024 KB
#define GB * 1024 MB
#define MAX_PARSE_SIZE (4 MB)

//const language_definition *definitions[] = {&cfml_definition, &json_definition, &php_definition, nullptr};
const language_definition *definitions[] = {&cfml_definition, &json_definition, nullptr};

static const language_definition *get_language_definition_by_filename(const char *filename)
{
    const language_definition *definition = nullptr;
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

            if (strcmp(definition_ext, filename_ext) == 0)
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

    if (strlen(text_background)) { write(STDOUT_FILENO, text_background, strlen(text_background)); clear_format = true; }
    if (strlen(text_color))      { write(STDOUT_FILENO, text_color, strlen(text_color));           clear_format = true; }
    if (strlen(text_flags))      { write(STDOUT_FILENO, text_flags, strlen(text_flags));           clear_format = true; }
    write(STDOUT_FILENO, text, len);
    if (clear_format)    write(STDOUT_FILENO, reset_ansi, strlen(reset_ansi));
}

static void print_recursive_token(const textparser_t handle, const char *text, const textparser_token_item *token)
{
    const struct textparser_token_item *child = nullptr;
    const language_definition *language = nullptr;
    char ansi_format_background[64] = {0, };
    char ansi_format_text_color[64] = {0, };
    char ansi_format_flags[64] = {0, };
    uint32_t text_background = 0;
    uint32_t text_color = 0;
    uint32_t text_flags = 0;
    size_t last_pos = 0;

    language = textparser_get_language(handle);

    text_background = language->tokens[token->token_id].text_background;
    if (text_background != TEXTPARSER_NOCOLOR) {
        snprintf(ansi_format_background, sizeof(ansi_format_background),
            "\33[48;2;%u;%u;%um",
            (text_background >>  0) & 0xff,
            (text_background >>  6) & 0xff,
            (text_background >> 16) & 0xff
        );
    }

    text_color = language->tokens[token->token_id].text_color;
    if (text_color != TEXTPARSER_NOCOLOR) {
        snprintf(ansi_format_text_color, sizeof(ansi_format_text_color),
            "\33[38;2;%u;%u;%um",
            (text_color >> 16) & 0xff,
            (text_color >>  8) & 0xff,
            (text_color >>  0) & 0xff
        );
    }

    text_flags = language->tokens[token->token_id].text_flags;
    if (text_flags & 0x01) strcat(ansi_format_flags, "\33[1m");
    if (text_flags & 0x02) strcat(ansi_format_flags, "\33[2m");
    if (text_flags & 0x04) strcat(ansi_format_flags, "\33[3m");
    if (text_flags & 0x08) strcat(ansi_format_flags, "\33[4m");

    child = token->child;

    last_pos = token->position;

    if (child) {
        while (child) {
            if (child->position > last_pos) {
                print_element(text + last_pos, child->position - last_pos, ansi_format_background, ansi_format_text_color, ansi_format_flags);
            }
            print_recursive_token(handle, text, child);
            last_pos = child->position + child->len;
            child = child->next;
        }

        if (token->position + token->len > last_pos) {
            print_element(text + last_pos, token->position + token->len - last_pos, ansi_format_background, ansi_format_text_color, ansi_format_flags);
        }

    } else {
        print_element(text + token->position, token->len, ansi_format_background, ansi_format_text_color, ansi_format_flags);
    }
}

int main(int argc, const char *argv[])
{
    const language_definition *language_def = nullptr;
    const textparser_token_item *token = nullptr;
    bool should_end_with_newline = false;
    const char *filename = nullptr;

    textparser_defer(handle);
    int res = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage ccat <text file>.\n");
        return EXIT_FAILURE;
    }

    filename = argv[1];

    res = textparser_openfile(filename, ADV_REGEX_TEXT_LATIN1, &handle);
    if (res) {
        fprintf(stderr, "Error opening file.\n");
        return EXIT_FAILURE;
    }

    language_def = get_language_definition_by_filename(filename);

    res = textparser_parse(handle, language_def);
    if (res) {
        fprintf(stderr, "Parsing failed.\n");
        return EXIT_FAILURE;
    }

    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);

    if ((text_size > 0)&&(text[text_size - 1] != '\n')) {
        should_end_with_newline = true;
    }

    token = textparser_get_first_token(handle);

    if (token) {
        size_t pos = 0;

        do {
            if (token->position > pos) {
                write(STDOUT_FILENO, text + pos, token->position - pos);
            }

            print_recursive_token(handle, text, token);

            pos = token->position + token->len;

            token = token->next;
        } while(token);

        if (text_size > pos) {
            write(STDOUT_FILENO, text + pos, text_size - pos);
        }
    } else {
        write(STDOUT_FILENO, text, text_size);
    }

    if (should_end_with_newline) {
        write(STDOUT_FILENO, "\n", 1);
    }

    return EXIT_SUCCESS;
}
