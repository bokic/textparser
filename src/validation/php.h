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
/* Register before executing the v2 grammar. Diagnostics are exposed through
 * textparser_validate_php after textparser_execute_language_grammar. */
EXPORT_PHP int textparser_php_register_validators(textparser_t handle);
EXPORT_PHP void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif
