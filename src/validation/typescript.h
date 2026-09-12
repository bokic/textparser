#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
 #define EXPORT_TYPESCRIPT __declspec(dllexport)
#else
 #define EXPORT_TYPESCRIPT __attribute__((visibility("default")))
#endif

EXPORT_TYPESCRIPT textparser_validation *textparser_validate_typescript(textparser_t handle);
EXPORT_TYPESCRIPT void textparser_validation_clear(textparser_validation *validation);
EXPORT_TYPESCRIPT int textparser_typescript_register_validators(textparser_t handle);

#ifdef __cplusplus
}
#endif
