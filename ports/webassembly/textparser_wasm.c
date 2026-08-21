#include <textparser.h>
#include <textparser-json.h>
#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

EMSCRIPTEN_KEEPALIVE
textparser_t wasm_textparser_create(void) {
    return NULL;
}

EMSCRIPTEN_KEEPALIVE
int wasm_textparser_openmem(const char *text, int len, textparser_t *out_handle) {
    return textparser_openmem(text, len, TEXTPARSER_ENCODING_UTF_8, out_handle);
}

EMSCRIPTEN_KEEPALIVE
int wasm_textparser_load_language_json(const char *json_str, textparser_language_definition **out_def) {
    return textparser_json_load_language_definition_from_string(json_str, out_def);
}

EMSCRIPTEN_KEEPALIVE
int wasm_textparser_parse(textparser_t handle, const textparser_language_definition *def) {
    return textparser_parse(handle, def);
}

EMSCRIPTEN_KEEPALIVE
void wasm_textparser_post_process(textparser_token_item **root, const textparser_language_definition *def) {
    textparser_post_process(root, def);
}

EMSCRIPTEN_KEEPALIVE
textparser_token_item *wasm_textparser_get_first_token(textparser_t handle) {
    return textparser_get_first_token(handle);
}

EMSCRIPTEN_KEEPALIVE
const char *wasm_textparser_parse_error(textparser_t handle) {
    return textparser_parse_error(handle);
}

EMSCRIPTEN_KEEPALIVE
size_t wasm_textparser_parse_error_position(textparser_t handle) {
    return textparser_parse_error_position(handle);
}

EMSCRIPTEN_KEEPALIVE
void wasm_textparser_close(textparser_t handle) {
    textparser_close(handle);
}

EMSCRIPTEN_KEEPALIVE
void wasm_textparser_free_language(textparser_language_definition *def) {
    textparser_free_language_definition(def);
}

EMSCRIPTEN_KEEPALIVE
int wasm_token_get_id(const textparser_token_item *token) {
    return token ? token->token_id : -1;
}

EMSCRIPTEN_KEEPALIVE
size_t wasm_token_get_position(const textparser_token_item *token) {
    return token ? token->position : 0;
}

EMSCRIPTEN_KEEPALIVE
size_t wasm_token_get_length(const textparser_token_item *token) {
    return token ? token->len : 0;
}

EMSCRIPTEN_KEEPALIVE
textparser_token_item *wasm_token_get_child(const textparser_token_item *token) {
    return token ? token->child : NULL;
}

EMSCRIPTEN_KEEPALIVE
textparser_token_item *wasm_token_get_next(const textparser_token_item *token) {
    return token ? token->next : NULL;
}

EMSCRIPTEN_KEEPALIVE
textparser_token_item *wasm_token_get_prev(const textparser_token_item *token) {
    return token ? token->prev : NULL;
}

EMSCRIPTEN_KEEPALIVE
textparser_token_item *wasm_token_get_parent(const textparser_token_item *token) {
    return token ? token->parent : NULL;
}

EMSCRIPTEN_KEEPALIVE
const char *wasm_token_get_error(const textparser_token_item *token) {
    return textparser_get_token_error(token);
}

EMSCRIPTEN_KEEPALIVE
const char *wasm_token_get_type_str(const textparser_language_definition *def, const textparser_token_item *token) {
    return textparser_get_token_type_str(def, token);
}

EMSCRIPTEN_KEEPALIVE
char *wasm_token_get_text(textparser_t handle, const textparser_token_item *token) {
    if (!handle || !token) return NULL;
    const char *full_text = textparser_get_text(handle);
    if (!full_text) return NULL;
    char *slice = malloc(token->len + 1);
    if (!slice) return NULL;
    memcpy(slice, full_text + token->position, token->len);
    slice[token->len] = '\0';
    return slice;
}

EMSCRIPTEN_KEEPALIVE
void wasm_free_token_text(char *text) {
    free(text);
}
