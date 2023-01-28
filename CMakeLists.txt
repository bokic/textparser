cmake_minimum_required(VERSION 3.15)

project(textparser LANGUAGES C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_C_STANDARD 23) # https://www.open-std.org/JTC1/SC22/WG14/www/projects

include_directories(BEFORE
    include
)

add_executable(textparser
    main.c
    src/textparser.c
    src/adv_regex.c
#    adv_regex.c
#    re.c

    include/textparser.h
    include/adv_regex.h
    cfml_definition.h
    json_definition.h
    php_definition.h
#    adv_regex.h
#    re.h
)

set_property(TARGET textparser PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

target_link_libraries(textparser
    utf8proc
    pcre2-8
)

#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
#set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -s")
