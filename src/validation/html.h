#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
 #ifdef BUILDING_TEXTPARSER_HTML_VAL
  #define EXPORT_HTML __declspec(dllexport)
 #else
  #define EXPORT_HTML __declspec(dllimport)
 #endif
#else
 #define EXPORT_HTML __attribute__((visibility("default")))
#endif

EXPORT_HTML textparser_validation *textparser_validate_html(textparser_t handle);
EXPORT_HTML void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif
