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
    EXPECT_TRUE(found.contains("CodeBlock_Start"));
    EXPECT_TRUE(found.contains("AssignOperator"));
    EXPECT_TRUE(found.contains("SingleString_Start"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
}

TEST(parse_Bash, parameter_expansions_and_special_variables) {
    auto tokens = TextParser(R"bash(
BUILD_TYPE="${BUILD_TYPE:-Release}"
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

    EXPECT_TRUE(found.contains("ParameterExpansion_Start"));
    EXPECT_TRUE(found.contains("ParameterExpansion_End"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Number"));
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

    EXPECT_TRUE(found.contains("CommandSubstitution_Start"));
    EXPECT_TRUE(found.contains("BacktickSubstitution_Start"));
    EXPECT_TRUE(found.contains("AnsiCString_Start"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
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

    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("ParameterExpansion_Start"));
    EXPECT_TRUE(found.contains("Variable"));
    // Verify DoubleStringContent does NOT trigger standalone Parenthesis_Start or CodeBlock_Start
    bool inside_str = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (!tokens[i].type) continue;
        std::string t = tokens[i].type;
        if (t == "DoubleString_Start") inside_str = true;
        else if (t == "DoubleString_End") inside_str = false;
        else if (inside_str && tokens[i].value.find("File (backup)") != std::string::npos) {
            EXPECT_NE(t, "Parenthesis_Start");
            EXPECT_NE(t, "CodeBlock_Start");
            EXPECT_NE(t, "ArrayIndex_Start");
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
    EXPECT_TRUE(found.contains("AssignOperator"));
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
    EXPECT_TRUE(found.contains("HereStart"));
    EXPECT_TRUE(found.contains("ProcessSubstitution_Start"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
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
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("AssignOperator"));
    EXPECT_TRUE(found.contains("AnsiCString_Start"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("CodeBlock_Start"));
    EXPECT_TRUE(found.contains("Parenthesis_Start"));
}

TEST(parse_Bash, prune_redundant_unprocessed_leaf) {
    auto plain_str = TextParser(R"bash("Ninja")bash", &bash_definition);
    ASSERT_GT(plain_str.count, 0);
    EXPECT_STREQ(plain_str[0].type, "DoubleString_Start");
    EXPECT_STREQ(plain_str[1].type, "DoubleStringContent");
    EXPECT_EQ(plain_str[1].value, "Ninja");
    EXPECT_STREQ(plain_str[2].type, "DoubleString_End");

    auto interp_str = TextParser(R"bash("Hello $USER!")bash", &bash_definition);
    ASSERT_GT(interp_str.count, 0);
    EXPECT_STREQ(interp_str[0].type, "DoubleString_Start");
    EXPECT_STREQ(interp_str[1].type, "DoubleStringContent");
    EXPECT_EQ(interp_str[1].value, "Hello ");
    EXPECT_STREQ(interp_str[2].type, "Variable");
    EXPECT_EQ(interp_str[2].value, "$USER");
    EXPECT_STREQ(interp_str[3].type, "DoubleStringContent");
    EXPECT_EQ(interp_str[3].value, "!");
    EXPECT_STREQ(interp_str[4].type, "DoubleString_End");
}

TEST(parse_Bash, path_and_arithmetic_expression) {
    auto tokens = TextParser(R"bash(
if [ -f build/compile_commands.json ]; then
    ./regenerate.sh
    cp /dev/null /tmp/out.log
fi

val=$(( 10 / 2 + 5 * 3 ))
(( count += 1 ))
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Path"));
    EXPECT_TRUE(found.contains("ArithmeticExpression_Start"));
    EXPECT_TRUE(found.contains("ArithmeticOperator"));

    // Find the Path `build/compile_commands.json` inside ArrayIndex brackets
    bool found_path_in_test = false;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type && std::string(tokens[i].type) == "ArrayIndex_Start") {
            for (size_t j = i + 1; j < tokens.count; ++j) {
                if (tokens[j].type && std::string(tokens[j].type) == "ArrayIndex_End") break;
                if (tokens[j].type && std::string(tokens[j].type) == "Path") {
                    EXPECT_EQ(tokens[j].value, "build/compile_commands.json");
                    found_path_in_test = true;
                }
            }
        }
    }
    EXPECT_TRUE(found_path_in_test);
}

TEST(parse_Bash, command_substitution_and_process_substitution) {
    auto tokens = TextParser(R"bash(
output=$(echo "Current dir: $(pwd)")
diff -u <(sort file1.txt) <(sort file2.txt)
tee >(logger -t myapp) > /dev/null
legacy=`hostname -s`
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("CommandSubstitution_Start"));
    EXPECT_TRUE(found.contains("ProcessSubstitution_Start"));
    EXPECT_TRUE(found.contains("BacktickSubstitution_Start"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("Variable"));
}

TEST(parse_Bash, extended_test_expression) {
    auto tokens = TextParser(R"bash(
if [[ "$str" =~ ^[0-9]+$ && -f "$file" || ! -d "$dir" ]]; then
    echo "Pattern matched"
fi
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("TestExpression_Start"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("TestSymbol"));
    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("Variable"));
}

TEST(parse_Bash, ansi_c_escapes_and_base_numbers) {
    auto tokens = TextParser(R"bash(
ansi_escapes=$'Alert:\a Bell:\b Esc:\e \E FormFeed:\f Unicode:\u0041 \U00000041 Control:\cA Hex:\x7f Octal:\033 Quest:\?'
num_b64=64#@_12
num_b16=16#DEAD_BEEF
num_b2=2#101010
num_oct=0755
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("AnsiCString_Start"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
}

TEST(parse_Bash, extended_builtins_and_redirections) {
    auto tokens = TextParser(R"bash(
mapfile -t lines < input.txt
readarray -t entries < <(find . -type f)
shopt -s extglob nullglob
pushd /tmp >/dev/null && popd >/dev/null
complete -F _my_comp my_cmd
disown -h %1
kill -9 "$pid"
cmd &> all_output.log
cmd &>> append_all.log
cat <<-EOF >| force_overwrite.txt
	tab-indented heredoc
EOF
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Redirection"));
    EXPECT_TRUE(found.contains("ProcessSubstitution_Start"));
    EXPECT_TRUE(found.contains("Variable"));
}

TEST(parse_Bash, corner_cases_and_edge_conditions) {
    auto tokens = TextParser(R"bash(
empty_str=""
empty_single=''
nested_param="${VAR:-$(echo "$DEFAULT")}"
nested_arith=$(( $(echo 10) + 5 ))
pipe_redir=cmd 2>&1 | tee /tmp/log
slice="${array[@]:1:2}"
escaped_quote="foo\"bar\\baz"
)bash", &bash_definition);

    ASSERT_GT(tokens.count, 0);
    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("DoubleString_Start"));
    EXPECT_TRUE(found.contains("DoubleString_End"));
    EXPECT_TRUE(found.contains("SingleString_Start"));
    EXPECT_TRUE(found.contains("SingleString_End"));
    EXPECT_TRUE(found.contains("ParameterExpansion_Start"));
    EXPECT_TRUE(found.contains("ParameterExpansion_End"));
    EXPECT_TRUE(found.contains("CommandSubstitution_Start"));
    EXPECT_TRUE(found.contains("CommandSubstitution_End"));
    EXPECT_TRUE(found.contains("ArithmeticExpression_Start"));
    EXPECT_TRUE(found.contains("ArithmeticExpression_End"));
    EXPECT_TRUE(found.contains("StringEscape"));
    EXPECT_TRUE(found.contains("Pipe"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
}

