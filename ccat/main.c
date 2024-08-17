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
#include <errno.h>

#define KB * 1024
#define MB * 1024 KB
#define GB * 1024 MB
#define MAX_PARSE_SIZE 4 MB

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

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    const language_definition *language_def = NULL;
    textparser_parser_state *line_state = NULL;
    const char *f_content = NULL;
    const char *filename = NULL;
    struct stat fd_stat;
    size_t f_size = 0;
    int fd = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage ccat <text file>.\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    filename = argv[1];

    fd = open(filename, O_RDONLY);
    if (fd <= 0) {
        fprintf(stderr, "Can't open [%s] file. Error: %s\n", filename, strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if (fstat(fd, &fd_stat)) {
        fprintf(stderr, "Failed to stat file. Error: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if (fd_stat.st_size > MAX_PARSE_SIZE) {
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    f_size = fd_stat.st_size;

    f_content = mmap(NULL, f_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (f_content == NULL) {
        fprintf(stderr, "Failed to map file. Error: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    language_def = get_language_definition_by_filename(filename);
    if (language_def == NULL)
    {
        fprintf(stderr, "No language definition found for file: %s\n", filename);
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    line_state = textparser_parse_state_new(NULL, 0);

    /*language_definition *def = NULL;
    int res = textparser_load_language_definition_from_json_file("json_definition.json", &def);
    textparser_free_language_definition(def);
    def = NULL;*/

    const char *line = f_content;
    do {
        textparser_line_parser_item *item = textparser_parse_line(line, ADV_REGEX_TEXT_LATIN1, line_state, language_def);

        item = NULL;

        // https://en.wikipedia.org/wiki/ANSI_escape_code#24-bit

    } while(1);

cleanup:
    if (line_state)
        textparser_parse_state_free(line_state);
    if (f_content)
        munmap ((void *)f_content, f_size);
    if (fd)
        close(fd);

    return ret;
}
