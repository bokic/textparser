#include <textparser.h>
#include <cfml_definition.json.h>
//#include <json_definition.json.h>
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

//const language_definition *definitions[] = {&cfml_definition, &json_definition, &php_definition, NULL};
//const language_definition *definitions[] = {&cfml_definition, &json_definition, NULL};
const language_definition *definitions[] = {&cfml_definition, NULL};


static const language_definition *get_language_definition_by_filename(const char *filename)
{
    const language_definition *definition = NULL;
    const char *definition_ext = NULL;
    int def_cnt = 0;

    const char *filename_ext = strrchr(filename, '.');
    if (filename_ext == NULL)
        return NULL;

    filename_ext++;

    while(definitions[def_cnt] != NULL)
    {
        int ext_cnt = 0;

        definition = definitions[def_cnt];

        while(definition->default_file_extensions[ext_cnt] != NULL)
        {
            definition_ext = definition->default_file_extensions[ext_cnt];

            if (strcmp(definition_ext, filename_ext) == 0)
                return definition;

            ext_cnt++;
        }

        def_cnt++;
    }

    return NULL;
}

static void print_recursive_token(textparser_t handle, const char *text, textparser_token_item *token)
{
    // ESC[38;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB foreground color
    // ESC[48;2;⟨r⟩;⟨g⟩;⟨b⟩m Select RGB background color

    const language_definition *language = NULL;
    char ansi_format_str[64] = {0, };
    bool have_format = false;
    uint32_t text_background = 0;
    uint32_t text_color = 0;
    uint32_t text_flags = 0;
    size_t pos = 0;

    language = textparser_get_language(handle);
    text_background = language->tokens[token->token_id].text_background;
    text_color = language->tokens[token->token_id].text_color;
    text_flags = language->tokens[token->token_id].text_flags;
    pos = token->position;

    if ((text_background)||(text_color)||(text_flags)) {

        have_format = true;
    }

    char *str = "\33[38;2;255;102;102m";
    const char *reset_ansi = "\33[0m";

    if (have_format) {
        write(STDOUT_FILENO, str, strlen(str));
    }

    write(STDOUT_FILENO, text + token->position, token->len);

    if (have_format) {
        write(STDOUT_FILENO, reset_ansi, strlen(reset_ansi));
    }
}

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    const language_definition *language_def = NULL;
    textparser_parser_state *line_state = NULL;
    textparser_token_item *token = NULL;
    bool should_end_with_newline = false;
    const char *f_content = NULL;
    const char *filename = NULL;
    struct stat fd_stat;
    size_t f_size = 0;
    int fd = 0;

    textparser_t handle = NULL;
    int res = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage ccat <text file>.\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    filename = argv[1];

    res = textparser_openfile(filename, ADV_REGEX_TEXT_LATIN1, &handle);
    if (res) {
        fprintf(stderr, "Error opening file.\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    res = textparser_parse(handle, &cfml_definition);
    if (res) {
        fprintf(stderr, "Parsing failed.\n");
        ret = EXIT_FAILURE;
        goto cleanup;
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

cleanup:
    if (handle) {
        textparser_close(handle);
    }

    return ret;
}
