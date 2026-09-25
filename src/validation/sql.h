#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
 #define EXPORT_SQL __declspec(dllexport)
#else
 #define EXPORT_SQL __attribute__((visibility("default")))
#endif

EXPORT_SQL int textparser_sql_register_validators(textparser_t handle);
EXPORT_SQL textparser_validation *textparser_validate_sql(textparser_t handle);
EXPORT_SQL void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif
