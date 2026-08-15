#pragma once

#include <textparser.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

static inline void validation_string_cleanup(char **str) {
    if (str && *str) {
        free(*str);
        *str = nullptr;
    }
}

#define validation_string_defer(var) char *var __attribute__((cleanup(validation_string_cleanup))) = nullptr

static inline char *dynamic_printf(const char *format, ...) {
    va_list args1, args2;

    va_start(args1, format);
    va_copy(args2, args1);

    int length = vsnprintf(nullptr, 0, format, args1);
    va_end(args1);

    if (length < 0) {
        va_end(args2);
        return nullptr;
    }

    char *buffer = malloc(length + 1);
    if (buffer == nullptr) {
        va_end(args2);
        return nullptr;
    }

    vsnprintf(buffer, length + 1, format, args2);
    va_end(args2);

    return buffer;
}

static inline void textparser_validation_item_add(enum textparser_validation_item_type type, textparser_validation **validation, char *text, size_t position, size_t length) {
    if (*validation == nullptr) {
        int initial_cap = 8;
        *validation = malloc(offsetof(textparser_validation, items) + sizeof(textparser_validation_item *) * initial_cap);
        if (*validation == nullptr) {
            free(text);
            return;
        }
        (*validation)->len = 0;
        (*validation)->capacity = initial_cap;
    } else if ((*validation)->len >= (*validation)->capacity) {
        int new_cap = (*validation)->capacity * 2;
        textparser_validation *new_val = realloc(*validation, offsetof(textparser_validation, items) + sizeof(textparser_validation_item *) * new_cap);
        if (new_val == nullptr) {
            free(text);
            return;
        }
        new_val->capacity = new_cap;
        *validation = new_val;
    }

    textparser_validation_item *item = malloc(sizeof(textparser_validation_item));
    if (item == nullptr) {
        free(text);
        return;
    }
    item->type = type;
    item->position = position;
    item->length = length;
    item->text = text;

    (*validation)->items[(*validation)->len] = item;
    (*validation)->len++;
}

static inline void validation_clear_internal(textparser_validation *validation) {
    if (validation != nullptr) {
        for (int i = 0; i < validation->len; i++) {
            free(validation->items[i]->text);
            free(validation->items[i]);
        }
        free(validation);
    }
}
