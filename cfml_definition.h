#ifndef CFML_DEFINITION_H
#define CFML_DEFINITION_H

#include"textparser.h"
#include <stddef.h>


static language_definition_t cfml_definition = {
    .name = "ColdFusion",
    .empty_segment_language = "html",
    .case_sensitivity = false,
    .default_file_extensions = (char *[]){"cfm", "cfc", 0},
    .starts_with = (char *[]){"ScriptTag", "OutputTag", "StartTag", "EndTag", "Comment", 0},
    .tokens = (textparse_token_t *[]) {
        //{.name = "boro", .start_string = "str", .end_string = "end", .only_start_tag = false, .multi_line = true},
        //{.name = NULL}
    }
};

#endif // CFML_DEFINITION_H
