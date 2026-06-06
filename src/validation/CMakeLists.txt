add_library(cfml SHARED
    cfml.c
)

target_include_directories(cfml PRIVATE
    ../../include
    ../../definitions
)

set_target_properties(cfml PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
    C_VISIBILITY_PRESET hidden
    OUTPUT_NAME "textparser_cfml"
    VERSION "${TEXTPARSER_VERSION_TAG}"
    SOVERSION "${TEXTPARSER_VERSION_TAG_SHORT}"
)

add_executable(cfml_test
    test.c
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
