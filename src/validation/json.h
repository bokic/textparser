#ifndef TEXTPARSER_VALIDATION_JSON_H
#define TEXTPARSER_VALIDATION_JSON_H

#include <textparser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_TEXTPARSER_JSON_VAL
        #define EXPORT_JSON __declspec(dllexport)
    #else
        #define EXPORT_JSON __declspec(dllimport)
    #endif
#else
    #define EXPORT_JSON __attribute__((visibility("default")))
#endif

/**
 * Register semantic legality handlers and token validators for JSON.
 */
EXPORT_JSON int textparser_json_register_validators(textparser_t handle);

/**
 * Extract JSON validation diagnostics as a textparser_validation struct.
 */
EXPORT_JSON textparser_validation *textparser_validate_json(textparser_t handle);

/**
 * Clear a textparser_validation structure.
 */
EXPORT_JSON void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif

#endif // TEXTPARSER_VALIDATION_JSON_H
