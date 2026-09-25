set(CMAKE_C_STANDARD 23)

add_library(cfml SHARED
    cfml.c
    cfml_grammar.c
)

target_compile_definitions(cfml PRIVATE BUILDING_TEXTPARSER_CFML_VAL)

target_include_directories(cfml PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(cfml PRIVATE
    libtextparser
)

set_target_properties(cfml PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_cfml"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(cfml_test
    test_cfml.c
    cfml.c
)

target_compile_definitions(cfml_test PRIVATE BUILDING_TEXTPARSER_CFML_VAL)

target_include_directories(cfml_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(cfml_test PRIVATE
    libtextparser
    cfml
)

set_target_properties(cfml_test PROPERTIES
    OUTPUT_NAME "validation_test"
)

add_library(php SHARED
    php.c
    php_grammar.c
)

target_compile_definitions(php PRIVATE BUILDING_TEXTPARSER_PHP_VAL)

target_include_directories(php PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(php PRIVATE
    libtextparser
)

set_target_properties(php PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_php"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(php_test
    test_php.c
    php.c
)

target_compile_definitions(php_test PRIVATE BUILDING_TEXTPARSER_PHP_VAL)

target_include_directories(php_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(php_test PRIVATE
    libtextparser
    php
)

set_target_properties(php_test PROPERTIES
    OUTPUT_NAME "php_validation_test"
)

add_library(html SHARED
    html.c
)

target_compile_definitions(html PRIVATE BUILDING_TEXTPARSER_HTML_VAL)

target_include_directories(html PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(html PRIVATE
    libtextparser
)

set_target_properties(html PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_html"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(html_test
    test_html.c
    html.c
)

target_compile_definitions(html_test PRIVATE BUILDING_TEXTPARSER_HTML_VAL)

target_include_directories(html_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(html_test PRIVATE
    libtextparser
    html
)

set_target_properties(html_test PROPERTIES
    OUTPUT_NAME "html_validation_test"
)

add_library(css SHARED
    css.c
)

target_compile_definitions(css PRIVATE BUILDING_TEXTPARSER_CSS_VAL)

target_include_directories(css PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(css PRIVATE
    libtextparser
)

set_target_properties(css PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_css"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(css_test
    test_css.c
    css.c
)

target_compile_definitions(css_test PRIVATE BUILDING_TEXTPARSER_CSS_VAL)

target_include_directories(css_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(css_test PRIVATE
    libtextparser
    css
)

set_target_properties(css_test PROPERTIES
    OUTPUT_NAME "css_validation_test"
)

add_library(typescript SHARED
    typescript.c
)

target_compile_definitions(typescript PRIVATE BUILDING_TEXTPARSER_TYPESCRIPT_VAL)

target_include_directories(typescript PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(typescript PRIVATE
    libtextparser
)

set_target_properties(typescript PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_typescript"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(typescript_test
    test_typescript.c
    typescript.c
)

target_compile_definitions(typescript_test PRIVATE BUILDING_TEXTPARSER_TYPESCRIPT_VAL)

target_include_directories(typescript_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(typescript_test PRIVATE
    libtextparser
    typescript
)

set_target_properties(typescript_test PROPERTIES
    OUTPUT_NAME "typescript_validation_test"
)

add_library(sql SHARED
    sql.c
)

target_compile_definitions(sql PRIVATE BUILDING_TEXTPARSER_SQL_VAL)

target_include_directories(sql PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(sql PRIVATE
    libtextparser
)

set_target_properties(sql PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_sql"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(sql_test
    test_sql.c
    sql.c
)

target_compile_definitions(sql_test PRIVATE BUILDING_TEXTPARSER_SQL_VAL)

target_include_directories(sql_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(sql_test PRIVATE
    libtextparser
    sql
)

set_target_properties(sql_test PROPERTIES
    OUTPUT_NAME "sql_validation_test"
)

add_library(json_val SHARED
    json.c
)

target_compile_definitions(json_val PRIVATE BUILDING_TEXTPARSER_JSON_VAL)

target_include_directories(json_val PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(json_val PRIVATE
    libtextparser
)

set_target_properties(json_val PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_json_val"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(json_test
    test_json.c
    json.c
)

target_compile_definitions(json_test PRIVATE BUILDING_TEXTPARSER_JSON_VAL)

target_include_directories(json_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(json_test PRIVATE
    libtextparser
    json_val
)

set_target_properties(json_test PROPERTIES
    OUTPUT_NAME "json_validation_test"
)

add_library(md SHARED
    md.c
)

target_compile_definitions(md PRIVATE BUILDING_TEXTPARSER_MD_VAL)

target_include_directories(md PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(md PRIVATE
    libtextparser
)

set_target_properties(md PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_md"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(md_test
    test_md.c
    md.c
)

target_compile_definitions(md_test PRIVATE BUILDING_TEXTPARSER_MD_VAL)

target_include_directories(md_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(md_test PRIVATE
    libtextparser
    md
)

set_target_properties(md_test PROPERTIES
    OUTPUT_NAME "md_validation_test"
)

add_library(r SHARED
    r.c
)

target_compile_definitions(r PRIVATE BUILDING_TEXTPARSER_R_VAL)

target_include_directories(r PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(r PRIVATE
    libtextparser
)

set_target_properties(r PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_r"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(r_test
    test_r.c
    r.c
)

target_compile_definitions(r_test PRIVATE BUILDING_TEXTPARSER_R_VAL)

target_include_directories(r_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(r_test PRIVATE
    libtextparser
    r
)

set_target_properties(r_test PROPERTIES
    OUTPUT_NAME "r_validation_test"
)

add_library(asm SHARED
    asm.c
)

target_compile_definitions(asm PRIVATE BUILDING_TEXTPARSER_ASM_VAL)

target_include_directories(asm PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(asm PRIVATE
    libtextparser
)

set_target_properties(asm PROPERTIES
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_asm"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(asm_test
    test_asm.c
    asm.c
)

target_compile_definitions(asm_test PRIVATE BUILDING_TEXTPARSER_ASM_VAL)

target_include_directories(asm_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(asm_test PRIVATE
    libtextparser
    asm
)

set_target_properties(asm_test PROPERTIES
    OUTPUT_NAME "asm_validation_test"
)

