#pragma once

#include"textparser.h"
#include <stddef.h>


static const language_definition_t cfml_definition = {
    .name = "ColdFusion",
    .empty_segment_language = "html",
    .case_sensitivity = false,
    .default_file_extensions = (char *[]) {"cfm", "cfc", NULL},
    .default_text_encoding = TEXTPARSE_LATIN_1,
    .starts_with = (int []) {0, 1, 2, 3, 4, -1},
    .tokens = (textparse_token_t[]) {
        {.name = "boro", .start_string = "str", .end_string = "end", .only_start_tag = false, .multi_line = true },
        {.name = "boro", .start_string = "str", .end_string = "end", .only_start_tag = false, .multi_line = true },
        {.name = "boro", .start_string = "str", .end_string = "end", .only_start_tag = false, .multi_line = true },
        {.name = "boro", .start_string = "str", .end_string = "end", .only_start_tag = false, .multi_line = true },
        {.name = NULL,   .start_string = NULL,  .end_string = NULL,  .only_start_tag = false, .multi_line = false},
    },
};
