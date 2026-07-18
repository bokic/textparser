#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
 #define EXPORT_PHP __declspec(dllexport)
#else
 #define EXPORT_PHP __attribute__((visibility("default")))
#endif

EXPORT_PHP textparser_validation *textparser_validate_php(textparser_t handle);
EXPORT_PHP void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif
