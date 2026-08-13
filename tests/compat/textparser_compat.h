#pragma once
#include <textparser.h>

#ifdef __cplusplus
extern "C" {
#endif

int textparser_parse_compat(textparser_t handle, const textparser_language_definition *definition);

#ifdef __cplusplus
}
#endif
