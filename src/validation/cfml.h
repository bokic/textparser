#pragma once

#include <textparser.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
 #define EXPORT_CFML __declspec(dllexport)
#else
 #define EXPORT_CFML __attribute__((visibility("default")))
#endif

EXPORT_CFML enum textparser_encoding textparser_get_encoding_cfml(textparser_t handle);
EXPORT_CFML textparser_validation *textparser_validate_cfml(textparser_t handle);
/* Register before executing the v2 grammar. Diagnostics are exposed through
 * textparser_validate_cfml after textparser_execute_language_grammar. */
EXPORT_CFML int textparser_cfml_register_validators(textparser_t handle);
EXPORT_CFML void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif

