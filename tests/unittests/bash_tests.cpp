#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <bash_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Bash, basic_bash_program) {
    auto tokens = TextParser(R"(
#!/bin/bash
# A simple bash script
function run_build() {
    local target=$1
    local verbose=true
    local count=100
    if [ $? -eq 0 ]; then
        echo -e "Building target: \"${target}\"\n"
        echo 'Build has started...\t'
    else
        echo "Build failed with exit code \$?"
    fi
}
)", &bash_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
}

TEST(parse_Bash, parameter_expansions_and_special_variables) {
    auto tokens = TextParser(R"bash(
echo ${#array[@]}
echo ${VAR:-default}
echo ${VAR:=default}
echo ${VAR:+alternate}
echo ${VAR:?error_msg}
echo ${VAR#prefix}
echo ${VAR##long_prefix}
echo ${VAR%suffix}
echo ${VAR%%long_suffix}
echo ${VAR/find/replace}
echo ${VAR//all/replace}
echo ${10}
echo $! $$ $# $@ $* $? $- $_ $0 $1
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Keyword"));
}

TEST(parse_Bash, command_substitution_and_ansi_strings) {
    auto tokens = TextParser(R"bash(
current_kernel=`uname -r`
formatted_date=$(date "+%Y-%m-%d")
ansi_text=$'hello\n\t\x1b[31mred\x1b[0m'
loc_text=$"internationalized text"
echo "kernel: `uname -r` and date: $(date)"
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("CommandSubstitution"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("StringEscape"));
}

TEST(parse_Bash, double_string_with_parentheses_and_braces) {
    auto tokens = TextParser(R"bash(
msg="File (backup) and {placeholder} and [item]"
file_path="prefix_${id}_suffix.txt"
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Variable"));
    // Verify DoubleString does NOT contain Parenthesis or CodeBlock
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type && std::string(tokens[i].type) == "DoubleString") {
            for (size_t j = 0; j < tokens[i].children; ++j) {
                EXPECT_STRNE(tokens[i][j].type, "Parenthesis");
                EXPECT_STRNE(tokens[i][j].type, "CodeBlock");
                EXPECT_STRNE(tokens[i][j].type, "ArrayIndex");
            }
        }
    }
}

TEST(parse_Bash, assignments_and_numbers) {
    auto tokens = TextParser(R"bash(
count=42
hex_val=0xFF
octal_val=0755
bin_val=0b1010
base_num=16#DEADBEEF
base2_num=2#10110011
count+=10
str+="more text"
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Operator"));
}

TEST(parse_Bash, arrays_heredocs_and_process_substitution) {
    auto tokens = TextParser(R"bash(
declare -a indexed_array=("apple" "banana" "cherry")
declare -A assoc_array=(["user"]="admin" ["id"]=1001)

cat <<EOF > output.txt
Heredoc with $USER and $(date)
EOF

diff -u <(ls -la dir1) <(ls -la dir2) || true
tee >(grep "ERR" > error.log) < input.txt

exec 3>&1 4>&2
exec 1>&3 2>&4
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("DoubleString"));
}

TEST(parse_Bash, complex_production_script) {
    auto tokens = TextParser(R"bash(
#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

readonly SCRIPT_NAME="${0##*/}"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly TMP_DIR="$(mktemp -d -t "${SCRIPT_NAME}.XXXXXX")"

cleanup() {
    local exit_code=$?
    echo "Cleaning up temporary files in ${TMP_DIR}..."
    rm -rf "${TMP_DIR}"
    trap - EXIT INT TERM
    exit "${exit_code}"
}
trap cleanup EXIT INT TERM

log() {
    local level="$1"; shift
    local ts
    ts="$(date +"%Y-%m-%d %H:%M:%S")"
    printf "[%s] [%-5s] %s\n" "${ts}" "${level}" "$*" >&2
}

VERBOSE=false
DRY_RUN=false
TARGET_ENV="production"
declare -a INPUT_FILES=()
declare -A METADATA=(
    ["author"]="DevOps"
    ["version"]="2.4.1"
)

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -e|--env)
            if [[ -n "${2:-}" && "$2" != -* ]]; then
                TARGET_ENV="$2"
                shift 2
            else
                log "ERROR" "Argument for $1 is missing"
                exit 1
            fi
            ;;
        --)
            shift
            INPUT_FILES+=("$@")
            break
            ;;
        *)
            INPUT_FILES+=("$1")
            shift
            ;;
    esac
done

coproc BG_WORKER {
    while read -r task; do
        if [[ "$task" == "STOP" ]]; then
            break
        fi
        echo "PROCESSED: $task"
    done
}

for file in "${INPUT_FILES[@]}"; do
    (
        subshell_var="PID_$$"
        log "INFO" "Worker ${subshell_var} processing ${file}"
    ) &
done

wait
echo "STOP" >&"${BG_WORKER[1]}"
)bash", &bash_definition);

    EXPECT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("CodeBlock"));
    EXPECT_TRUE(found.contains("Parenthesis"));
}


