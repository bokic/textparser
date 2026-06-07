#pragma once

typedef enum {
    CFML_END_TAG_REQUIRED, // must have ending tag </...
    CFML_END_TAG_OPTIONAL, // can have ending tag, or can be self-closing
    CFML_END_TAG_FORBIDDEN // should not have ending tag
} cfml_end_tag_type;

typedef enum {
    CFML_PARAMS_NONE,        // cftags that do not have any parameters
    CFML_PARAMS_KEY_VALUE,   // e.g. <cfmail to="foo">
    CFML_PARAMS_EXPRESSION   // e.g. <cfif condition>, <cfset x = 1>
} cfml_params_type;

typedef enum {
    CFML_ATTR_TYPE_ANY,
    CFML_ATTR_TYPE_STRING,
    CFML_ATTR_TYPE_NUMBER,
    CFML_ATTR_TYPE_BOOLEAN,
    CFML_ATTR_TYPE_ARRAY,
    CFML_ATTR_TYPE_STRUCT,
    CFML_ATTR_TYPE_QUERY,
    CFML_ATTR_TYPE_OBJECT,
    CFML_ATTR_TYPE_VOID,
    CFML_ATTR_TYPE_DATE
} cfml_attr_type;

typedef enum {
    CFML_MASK_LITERAL = 1 << 0,
    CFML_MASK_VARIABLE = 1 << 1,
    CFML_MASK_EXPRESSION = 1 << 2,
    CFML_MASK_ALL = CFML_MASK_LITERAL | CFML_MASK_VARIABLE | CFML_MASK_EXPRESSION
} cfml_mask_type;

typedef struct {
    const char *name;
    bool required;
    cfml_attr_type type;
} cfml_function_parameter_info;

typedef struct {
    const char *name;
    cfml_attr_type return_type;
    const cfml_function_parameter_info *parameters;
} cfml_function_info;

typedef struct {
    const char *name;
    bool required;
    cfml_attr_type type;
    const char *value;
    int mask_kind;
} cfml_attribute_info;

typedef struct {
    const cfml_attribute_info *attributes;
} cfml_attribute_combo;

typedef struct {
    const char *name;
    cfml_end_tag_type end_tag_type;
    cfml_params_type params_type;
    const cfml_attribute_combo *attribute_combos;
} cfml_tag_info;
