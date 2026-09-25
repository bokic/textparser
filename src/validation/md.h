#ifndef TEXTPARSER_VALIDATION_MD_H
#define TEXTPARSER_VALIDATION_MD_H

#include <textparser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_TEXTPARSER_MD_VAL
        #define EXPORT_MD __declspec(dllexport)
    #else
        #define EXPORT_MD __declspec(dllimport)
    #endif
#else
    #define EXPORT_MD __attribute__((visibility("default")))
#endif

/**
 * Register semantic legality handlers and token validators for Markdown.
 */
EXPORT_MD int textparser_md_register_validators(textparser_t handle);

/**
 * Extract Markdown validation diagnostics as a textparser_validation struct.
 */
EXPORT_MD textparser_validation *textparser_validate_md(textparser_t handle);

/**
 * Clear a textparser_validation structure.
 */
EXPORT_MD void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif

#endif // TEXTPARSER_VALIDATION_MD_H
