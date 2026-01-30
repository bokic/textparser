#pragma once

#define TEXTPARSER_LOGGING_LEVEL_VERBOSE 0
#define TEXTPARSER_LOGGING_LEVEL_INFO    1
#define TEXTPARSER_LOGGING_LEVEL_DEBUG   2
#define TEXTPARSER_LOGGING_LEVEL_WARNING 3
#define TEXTPARSER_LOGGING_LEVEL_ERROR   4
#define TEXTPARSER_LOGGING_LEVEL_FATAL   5
#define TEXTPARSER_LOGGING_LEVEL_NONE    6

#ifndef TEXTPARSER_LOGGING_LEVEL
#define TEXTPARSER_LOGGING_LEVEL TEXTPARSER_LOGGING_LEVEL_NONE
#endif

#if TEXTPARSER_LOGGING_LEVEL < TEXTPARSER_LOGGING_LEVEL_NONE
#include <stdio.h>
#endif

#define ANSI_RESET           "\033[0m"
#define ANSI_BOLD            "\033[1m"
#define ANSI_RED_BOLD_BG     "\033[0;1;41m"
#define ANSI_GREEN_BOLD_BG   "\033[0;1;42m"
#define ANSI_YELLOW_BOLD_BG  "\033[0;1;43m"
#define ANSI_BLUE_BOLD_BG    "\033[0;1;44m"
#define ANSI_MAGENTA_BOLD_BG "\033[0;1;45m"
#define ANSI_RED_FG          "\033[0;31m"
#define ANSI_GREEN_FG        "\033[0;32m"
#define ANSI_YELLOW_FG       "\033[0;33m"
#define ANSI_BLUE_FG         "\033[0;34m"
#define ANSI_MAGENTA_FG      "\033[0;35m"

#define LOGV(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_VERBOSE) { printf(ANSI_BOLD            " [V] " ANSI_RESET      " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
#define LOGI(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_INFO)    { printf(ANSI_GREEN_BOLD_BG   " [I] " ANSI_GREEN_FG   " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
#define LOGD(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_DEBUG)   { printf(ANSI_BLUE_BOLD_BG    " [D] " ANSI_BLUE_FG    " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
#define LOGW(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_WARNING) { printf(ANSI_YELLOW_BOLD_BG  " [W] " ANSI_YELLOW_FG  " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
#define LOGE(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_ERROR)   { printf(ANSI_RED_BOLD_BG     " [E] " ANSI_RED_FG     " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
#define LOGF(text, ...) if (TEXTPARSER_LOGGING_LEVEL <= TEXTPARSER_LOGGING_LEVEL_FATAL)   { printf(ANSI_MAGENTA_BOLD_BG " [F] " ANSI_MAGENTA_FG " %s():%d", __func__, __LINE__); printf(" - " text ANSI_RESET "\n"  __VA_OPT__(,) __VA_ARGS__); fflush(stdout); }
