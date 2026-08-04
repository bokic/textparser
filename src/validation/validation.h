#pragma once

#include <stdlib.h>

static inline void validation_string_cleanup(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

#define validation_string_defer(var) char *var __attribute__((cleanup(validation_string_cleanup))) = NULL
