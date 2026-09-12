#include <stdlib.h>
#include <textparser.h>
#include <typescript_definition.json.h>
#include "typescript.h"

#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    textparser_t handle = nullptr;

    if (argc != 2) {
        printf("Usage: %s <ts_block>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int res = textparser_openmem(argv[1], strlen(argv[1]), TEXTPARSER_ENCODING_UTF_8, &handle);
    if (res != 0) {
        printf("Failed to open memory. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        return EXIT_FAILURE;
    }

    textparser_typescript_register_validators(handle);

    res = textparser_parse(handle, &typescript_definition);
    if (res != 0) {
        printf("Failed to parse memory. Error `%s`, code %d\n", textparser_parse_error(handle), res);
        textparser_close(handle);
        return EXIT_FAILURE;
    }

    textparser_validation *validation = textparser_validate_typescript(handle);
    if (validation != nullptr) {
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

    printf("Successfully parsed memory.\n");
    textparser_close(handle);
    return EXIT_SUCCESS;
}
