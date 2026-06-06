#pragma once

#include <textparser.h>


EXPORT_TEXTPARSER enum textparser_encoding textparser_get_encoding_cfml(textparser_t handle);
EXPORT_TEXTPARSER textparser_validation *textparser_validate_cfml(textparser_t handle);
EXPORT_TEXTPARSER void textparser_validation_clear(textparser_validation *validation);
