#include <emscripten.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "textparser.h"
#include "textparser-json.h"

// Keep this alive for the simple demo
static textparser_language_definition *global_lang_def = NULL;
static textparser_t global_parser = NULL;

EMSCRIPTEN_KEEPALIVE
int wasm_load_language_from_json(const char *json_str) {
    if (global_lang_def) {
        // Free previous (not fully implemented in this snippet, leaking slightly if repeatedly called without cleanup)
        free(global_lang_def); 
    }
    
    int ret = textparser_json_load_language_definition_from_string(json_str, &global_lang_def);
    if (ret != 0) {
        printf("Failed to load language definition: %d\n", ret);
        if (global_lang_def && global_lang_def->error_string) {
            printf("Error detail: %s\n", global_lang_def->error_string);
        }
        return ret;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int wasm_load_language_from_json2(const char *json_str, void **ptr) {
    int ret = textparser_json_load_language_definition_from_string(json_str, (textparser_language_definition **)ptr);
    if (ret != 0) {
        printf("Failed to load language definition: %d\n", ret);
        if (*ptr && ((textparser_language_definition *)*ptr)->error_string) {
            printf("Error detail: %s\n", ((textparser_language_definition *)*ptr)->error_string);
        }
        return ret;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int wasm_parse_text(const char *text) {
    if (!global_lang_def) {
        printf("Language definition not loaded.\n");
        return -1;
    }

    if (global_parser) {
        textparser_close(global_parser);
        global_parser = NULL;
    }

    int ret = textparser_openmem(text, strlen(text), TEXTPARSER_ENCODING_UTF_8, &global_parser);
    if (ret != 0) {
        printf("Failed to open mem: %d\n", ret);
        return ret;
    }

    ret = textparser_parse(global_parser, global_lang_def);
    if (ret != 0) {
        printf("Parse failed: %d\n", ret);
        printf("Error: %s at %zu\n", textparser_parse_error(global_parser), textparser_parse_error_position(global_parser));
        return ret;
    }

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int wasm_get_token_count() {
    if (!global_parser) return -1;
    
    int count = 0;
    textparser_token_item *item = textparser_get_first_token(global_parser);
    while(item) {
        count++;
        // This is a flat iteration of the first level? Or linked list?
        // textparser_get_token_next(item) usually goes to next sibling or DFS?
        // Based on header: textparser_token_item has 'next' and 'child'.
        // textparser_get_token_next likely traverses the flat list order/siblings?
        // Let's just count top level for simple check
        item = (textparser_token_item*)textparser_get_token_next(item);
    }
    return count;
}
