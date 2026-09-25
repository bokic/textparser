#include <stdlib.h>
#include <textparser.h>
#include <md_definition.json.h>
#include "md.h"

#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    textparser_t handle = NULL;

    if (argc != 2) {
        printf("Usage: %s <markdown_string>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int res = textparser_openmem(argv[1], strlen(argv[1]), TEXTPARSER_ENCODING_UTF_8, &handle);
    if (res != 0) {
        printf("Failed to open memory. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        return EXIT_FAILURE;
    }

    res = textparser_parse(handle, &md_definition);
    if (res != 0) {
        printf("Failed to parse memory. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        textparser_close(handle);
        return EXIT_FAILURE;
    }

    res = textparser_md_register_validators(handle);
    if (res != 0) {
        printf("Failed to register Markdown validators. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        textparser_close(handle);
        return EXIT_FAILURE;
    }

    textparser_match_result match = {0};
    res = textparser_execute_language_grammar(handle, &md_definition, &match);
    if (res != 0) {
        printf("Failed to execute Markdown grammar. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        textparser_close(handle);
        return EXIT_FAILURE;
    }

    textparser_validation *validation = textparser_validate_md(handle);
    if (validation != NULL) {
        for (int c = 0; c < validation->len; c++) {
            switch (validation->items[c]->type)
            {
                case TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR:
                    printf("\033[41m[E]\033[0m ");
                    break;
                case TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING:
                    printf("\033[43m[W]\033[0m ");
                    break;
                case TEXTPARSER_VALIDATION_ITEM_TYPE_INFO:
                    printf("\033[42m[I]\033[0m ");
                    break;
            }
            printf("%s\n", validation->items[c]->text);
        }
        textparser_validation_clear(validation);
        textparser_close(handle);
        return EXIT_SUCCESS;
    }

    textparser_close(handle);
    printf("Successfully parsed memory.\n");
    return EXIT_SUCCESS;
}
