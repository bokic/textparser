#ifndef TEXTPARSER_VALIDATION_ASM_H
#define TEXTPARSER_VALIDATION_ASM_H

#include <textparser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef BUILDING_TEXTPARSER_ASM_VAL
        #define EXPORT_ASM __declspec(dllexport)
    #else
        #define EXPORT_ASM __declspec(dllimport)
    #endif
#else
    #define EXPORT_ASM __attribute__((visibility("default")))
#endif

/**
 * Register semantic legality handlers and token validators for Assembly.
 */
EXPORT_ASM int textparser_asm_register_validators(textparser_t handle);

/**
 * Extract Assembly validation diagnostics as a textparser_validation struct.
 */
EXPORT_ASM textparser_validation *textparser_validate_asm(textparser_t handle);

/**
 * Clear a textparser_validation structure.
 */
EXPORT_ASM void textparser_validation_clear(textparser_validation *validation);

#ifdef __cplusplus
}
#endif

#endif // TEXTPARSER_VALIDATION_ASM_H
