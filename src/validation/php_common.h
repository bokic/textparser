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

typedef struct {
    const char *name;
    php_attr_type return_type;
    const php_function_parameter_info *parameters;
} php_function_info;
