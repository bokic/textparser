#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
 #define EXPORT_CSS __declspec(dllexport)
#else
 #define EXPORT_CSS __attribute__((visibility("default")))
#endif

EXPORT_CSS textparser_validation *textparser_validate_css(textparser_t handle);
EXPORT_CSS void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif
