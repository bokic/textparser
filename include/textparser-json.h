#pragma once

#include <textparser.h>
#include <adv_regex.h>
#include <stdbool.h>
#include <stddef.h>


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
