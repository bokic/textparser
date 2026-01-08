#pragma once

#include <textparser.h>
#include <adv_regex.h>
#include <stdbool.h>
#include <stddef.h>


enum {
    TEXTPARSER_JSON_NO_ERROR,
    TEXTPARSER_JSON_ROOT_OBJ_IS_NULL,
    TEXTPARSER_JSON_OUT_OF_MEMORY,
    TEXTPARSER_JSON_NAME_NOT_FOUND,
    TEXTPARSER_JSON_VERSION_NOT_FOUND,
    TEXTPARSER_JSON_EMPTY_SEGMENT_LANGUAGE_NOT_FOUND,
    TEXTPARSER_JSON_CASE_SENSITIVITY_NOT_FOUND,
    TEXTPARSER_JSON_FILE_EXTENSIONS_NOT_FOUND,
    TEXTPARSER_JSON_ENCODING_NOT_FOUND,
    TEXTPARSER_JSON_STARTS_WITH_NOT_FOUND,
    TEXTPARSER_JSON_STARTS_WITH_NOT_ARRAY,
    TEXTPARSER_JSON_TOKENS_NOT_FOUND,
    TEXTPARSER_JSON_TOKENS_NOT_ARRAY,
    TEXTPARSER_JSON_STARTS_WITH_IS_EMPTY,
    TEXTPARSER_JSON_STARTS_WITH_ELEMENT_NOT_STRING,
    TEXTPARSER_JSON_NESTED_TOKENS_NOT_ARRAY,
    TEXTPARSER_JSON_NESTED_TOKENS_IS_EMPTY,
    TEXTPARSER_JSON_NESTED_TOKENS_ELEMENT_NOT_STRING
};

#ifdef libtextparser_json_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_TEXTPARSER_JSON __declspec(dllexport)
 #else
  #define EXPORT_TEXTPARSER_JSON __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_TEXTPARSER_JSON
#endif

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_TEXTPARSER_JSON int textparser_json_load_language_definition_from_json_file(const char *pathname, language_definition **definition);
EXPORT_TEXTPARSER_JSON int textparser_json_load_language_definition_from_string(const char *text, language_definition **definition);

#ifdef __cplusplus
}
#endif
