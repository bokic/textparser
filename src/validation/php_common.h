#pragma once

#include <stdbool.h>

typedef enum {
    PHP_ATTR_TYPE_ANY,
    PHP_ATTR_TYPE_STRING,
    PHP_ATTR_TYPE_INT,
    PHP_ATTR_TYPE_FLOAT,
    PHP_ATTR_TYPE_BOOLEAN,
    PHP_ATTR_TYPE_ARRAY,
    PHP_ATTR_TYPE_OBJECT,
    PHP_ATTR_TYPE_RESOURCE,
    PHP_ATTR_TYPE_CALLABLE,
    PHP_ATTR_TYPE_MIXED,
    PHP_ATTR_TYPE_VOID,
    PHP_ATTR_TYPE_NULL,
    PHP_ATTR_TYPE_FALSE
} php_attr_type;

typedef struct {
    const char *name;
    bool required;
    bool is_variadic;
    php_attr_type type;
} php_function_parameter_info;

typedef struct php_function_info {
    const char *name;
    php_attr_type return_type;
    const php_function_parameter_info *parameters;
    /* Alternative signatures from conditional PHP builds; NULL ends the list. */
    const struct php_function_info *alternative;
} php_function_info;

/* Case-folded built-in lookup against the generated signature table. Returns
 * NULL when the name is not a known built-in. */
const php_function_info *php_find_function(const char *name);

/* Returns a malloc'd arity diagnostic for a call to a known built-in, or NULL
 * when the argument count is accepted by at least one signature variant. */
char *php_function_arity_error(const php_function_info *info, const char *name, int arg_count);
