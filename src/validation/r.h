#ifndef TEXTPARSER_VALIDATION_R_H
#define TEXTPARSER_VALIDATION_R_H

#include <textparser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_TEXTPARSER_R_VAL
        #define EXPORT_R __declspec(dllexport)
    #else
        #define EXPORT_R __declspec(dllimport)
    #endif
#else
    #define EXPORT_R __attribute__((visibility("default")))
#endif

/**
 * Register semantic legality handlers and token validators for R.
 */
EXPORT_R int textparser_r_register_validators(textparser_t handle);

/**
 * Extract R validation diagnostics as a textparser_validation struct.
 */
EXPORT_R textparser_validation *textparser_validate_r(textparser_t handle);

/**
 * Clear a textparser_validation structure.
 */
EXPORT_R void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif

#endif // TEXTPARSER_VALIDATION_R_H
