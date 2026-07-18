#pragma once

#include <stdbool.h>

typedef struct {
    const char *name;
    const char *const *attributes;
    int attribute_count;
} html_tag_info;
