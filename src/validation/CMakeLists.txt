set(CMAKE_C_STANDARD 23)

add_library(cfml SHARED
    cfml.c
)

target_include_directories(cfml PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(cfml PRIVATE
    libtextparser
)

set_target_properties(cfml PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_cfml"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(cfml_test
    test_cfml.c
    cfml.c
)

target_include_directories(cfml_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(cfml_test PRIVATE
    libtextparser
    cfml
)

set_target_properties(cfml_test PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    OUTPUT_NAME "validation_test"
)

add_library(php SHARED
    php.c
)

target_include_directories(php PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(php PRIVATE
    libtextparser
)

set_target_properties(php PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_php"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(php_test
    test_php.c
    php.c
)

target_include_directories(php_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(php_test PRIVATE
    libtextparser
    php
)

set_target_properties(php_test PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    OUTPUT_NAME "php_validation_test"
)

add_library(html SHARED
    html.c
)

target_include_directories(html PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(html PRIVATE
    libtextparser
)

set_target_properties(html PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_html"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(html_test
    test_html.c
    html.c
)

target_include_directories(html_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(html_test PRIVATE
    libtextparser
    html
)

set_target_properties(html_test PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    OUTPUT_NAME "html_validation_test"
)

add_library(css SHARED
    css.c
)

target_include_directories(css PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(css PRIVATE
    libtextparser
)

set_target_properties(css PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_css"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(css_test
    test_css.c
    css.c
)

target_include_directories(css_test PRIVATE
    ../../include
    ../../definitions
)

target_link_libraries(css_test PRIVATE
    libtextparser
    css
)

set_target_properties(css_test PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    OUTPUT_NAME "css_validation_test"
)
