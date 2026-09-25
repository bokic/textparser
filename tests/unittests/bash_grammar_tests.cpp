#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <cstring>
#include <string>

namespace {

struct BashGrammarFixture : testing::Test {
    textparser_language_definition *definition = nullptr;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                      "definitions/bash_definition.json", &definition),
                  TEXTPARSER_JSON_NO_ERROR);
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
    }

    void TearDown() override {
        if (definition != nullptr) {
            textparser_free_language_definition(definition);
            definition = nullptr;
        }
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(definition), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(definition, &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                result = {};
                result.status = TEXTPARSER_MATCH_NO;
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    const textparser_node *find(const textparser_node *node, const char *kind) {
        if (!node) return nullptr;
        const char *name = textparser_grammar_node_name(parser.get(), node);
        if (name && std::strcmp(name, kind) == 0) return node;
        for (auto *child = node->child; child; child = child->next) {
            if (auto *found = find(child, kind)) return found;
        }
        return nullptr;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

TEST_F(BashGrammarFixture, parses_common_constructs) {
    for (const char *source : {
             "echo hello\n",
             "x=1\n",
             "if [ \"$x\" = \"1\" ]; then echo yes; fi\n",
             "for i in 1 2 3; do echo $i; done\n",
             "while read -r line; do echo \"$line\"; done\n",
             "case \"$1\" in -h|--help) exit 0;; esac\n",
             "arr=(a b c)\n",
             "echo \"hello $world\"\n",
             "echo 'single quoted'\n",
             "x=1 # trailing comment\n"})
        EXPECT_NE(parse_source(source), nullptr);
    // A shebang line is pure trivia, so it matches zero tokens.
    parse_source("#!/usr/bin/env bash\n");
}

TEST_F(BashGrammarFixture, parses_expansions_and_substitutions) {
    for (const char *source : {
             "echo ${VAR}\n",
             "echo ${VAR:-default}\n",
             "echo ${VAR:?error}\n",
             "echo ${VAR#prefix}\n",
             "echo ${VAR##long_prefix}\n",
             "echo ${VAR%%long_suffix}\n",
             "echo ${VAR/find/replace}\n",
             "echo ${#VAR}\n",
             "echo ${arr[@]}\n",
             "echo ${10}\n",
             "echo $! $$ $# $@ $* $? $- $_ $0 $1\n",
             "echo $(date)\n",
             "echo `date`\n",
             "echo <(ls)\n",
             "echo >(grep ERR)\n",
             "[[ -n \"$x\" && \"$x\" == \"y\" ]]\n",
             "(( x = 1 + 2 * 3 ))\n"})
        EXPECT_NE(parse_source(source), nullptr);
}

TEST_F(BashGrammarFixture, parses_full_script) {
    const char *script =
        "#!/usr/bin/env bash\n"
        "set -euo pipefail\n"
        "readonly SCRIPT_NAME=\"${0##*/}\"\n"
        "readonly TMP_DIR=\"$(mktemp -d -t \"${SCRIPT_NAME}.XXXXXX\")\"\n"
        "cleanup() {\n"
        "    local exit_code=$?\n"
        "    echo \"Cleaning up ${TMP_DIR}...\"\n"
        "    rm -rf \"${TMP_DIR}\"\n"
        "    exit \"${exit_code}\"\n"
        "}\n"
        "trap cleanup EXIT INT TERM\n"
        "declare -a INPUT_FILES=()\n"
        "while [[ $# -gt 0 ]]; do\n"
        "    INPUT_FILES+=(\"$1\")\n"
        "    shift\n"
        "done\n";
    EXPECT_NE(parse_source(script), nullptr);
}

TEST_F(BashGrammarFixture, builds_structured_cst) {
    auto *root = parse_source("echo \"${arr[@]}\"\n");
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "DoubleString"), nullptr);
    EXPECT_NE(find(root, "ParameterExpansion"), nullptr);
    EXPECT_NE(find(root, "ArrayIndex"), nullptr);

    auto *subshell = parse_source("echo $(date)\n");
    ASSERT_NE(subshell, nullptr);
    EXPECT_NE(find(subshell, "CommandSubstitution"), nullptr);
}

TEST_F(BashGrammarFixture, comment_only_input_is_accepted) {
    // All-trivia input matches zero tokens (status OK, no node).
    parse_source("# just a comment\n");
}

TEST_F(BashGrammarFixture, hash_is_not_a_comment_inside_parameter_expansion) {
    auto *root = parse_source("echo ${#VAR}\n");
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "ParameterExpansion"), nullptr);
}

// Full command grammar (definitions/bash_definition.json) built
// on the promoted tokenization profile. Covers the constructs validated against
// `bash -n`.
struct BashFullGrammarFixture : testing::Test {
    textparser_language_definition *definition = nullptr;

    void SetUp() override {
        ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                      "definitions/bash_definition.json", &definition),
                  TEXTPARSER_JSON_NO_ERROR);
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
    }

    void TearDown() override {
        if (definition != nullptr) {
            textparser_free_language_definition(definition);
            definition = nullptr;
        }
    }

    bool parses(const char *source) {
        SCOPED_TRACE(source);
        parser.reset();
        if (parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8) != 0) return false;
        if (parser.parse(definition) != 0) return false;
        textparser_match_result result = {};
        if (parser.execute_language_grammar(definition, &result) != 0) return false;
        if (result.status != TEXTPARSER_MATCH_OK) return false;
        const textparser_lex_token *remaining = nullptr;
        int peek = textparser_lexer_peek(parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
        if (peek == 0 && remaining != nullptr) return false;
        return textparser_get_diagnostic_count(parser.get()) == 0;
    }

    void accepts(std::initializer_list<const char *> sources) {
        for (const char *source : sources) EXPECT_TRUE(parses(source)) << source;
    }

    textparser::Parser parser;
};

TEST_F(BashFullGrammarFixture, parses_commands_lists_and_redirections) {
    accepts({
        "echo hello world\n",
        "x=1\n",
        "x=1 y=2 env\n",
        "x+=1\n",
        "export PATH=\"$PATH:/opt/bin\"\n",
        "echo a | grep b | wc -l\n",
        "echo a && echo b || echo c\n",
        "cmd1 & cmd2 &\n",
        "cmd >/dev/null 2>&1\n",
        "cmd 2>&1 | tee log\n",
        "cat < file\n",
        "cmd > out 2> err\n",
        "echo a |& grep a\n",
        "time cmd\n",
        "! false\n",
        "echo {a,b} {1..5} {a,b}{c,d}\n",
        "echo {/usr,,/usr/local}/lib*\n",
        "find . -exec mv {} /tmp \\;\n",
        "done=false\n",
        "if=1 then=2\n",
        "while getopts vhao:d:-: OPT; do echo ok; done\n",
        "echo a;\necho b;\n",
        "f() {\n  return 0;\n  return 0;\n}\n",
        "echo a\n\n\necho b\n",
        "echo a\n# comment line\n\necho b\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_control_flow) {
    accepts({
        "if true; then echo yes; fi\n",
        "if true; then echo a; elif false; then echo b; else echo c; fi\n",
        "if [ -f file ]; then echo yes; else echo no; fi\n",
        "if [ \"$i\" = 2 ]; then continue; fi\n",
        "if (( $EUID != 0 )); then echo a; fi\n",
        "if ! expr \"$X\" : \"[[:digit:]]\\+$\" >/dev/null 2>&1; then X=80; fi\n",
        "for i in 1 2 3; do echo $i; done\n",
        "for arg do echo $arg; done\n",
        "for f in *.txt; do echo \"$f\"; done\n",
        "for ((i=0; i<3; i++)); do echo $i; done\n",
        "for ((i=0;i<3;i++)); do echo $i; done\n",
        "while read -r line; do echo \"$line\"; done < input.txt\n",
        "until false; do break; done\n",
        "select opt in a b c; do break; done\n",
        "while getopts :acd:hlpmus opt; do case $opt in a) x=1 ;; esac; done\n",
        "if ( test $# -eq 0 ) then echo yes; fi\n",
        "if ( test $# -eq 0 ); then echo yes; fi\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_case_patterns) {
    accepts({
        "case $1 in a) echo a ;; esac\n",
        "case $x in\n  a) echo a ;;\n  b|c) echo bc ;;\n  *) echo other ;;\nesac\n",
        "case $1 in -h|--help) exit 0;; esac\n",
        "case $x in -*) echo opt ;; esac\n",
        "case $x in --prefix=*) prefix=1 ;; esac\n",
        "case $x in *[!${2}]* | '') return 1 ;; esac\n",
        "case $file in [[:alpha:]]*-*) return ;; esac\n",
        "case $1 in a) echo a ;; *) echo default esac\n",
        "case $x in --color=@(never|false)) echo ok ;; esac\n",
        "case $x in --color@(|=*)) echo ok ;; esac\n",
        "case \"$prog\" in *cmp) comp=${CMP-cmp} ;; *) comp=${DIFF-diff} ;; esac\n",
        "case $x in a) ;; b) echo b ;; esac\n",
        "case $x in\n  a)\n  ;;\n  b)\n  ;;\n  *)\n  echo default\n  ;;\nesac\n",
        "case $option in (--binary-*=* | --[lm]a*=* | --reg*=*) ;; esac\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_functions_groups_and_subshells) {
    accepts({
        "foo() { echo hi; }\n",
        "foo()\n{\n  local x=1\n  echo $x\n}\n",
        "help() {\n  echo hi\n}\n",
        "function help {\n  echo hi\n}\n",
        "function bar { echo bye; }\n",
        "function f() { echo x; }\n",
        "(cd /tmp && pwd)\n",
        "{ echo a; echo b; }\n",
        "{\n  echo a\n}\n",
        "cmd && {\n  echo a\n}\n",
        "{ echo a; echo b; } | wc -l\n",
        "(cd /tmp; ls) > out.txt\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_expansions_and_substitutions) {
    accepts({
        "echo \"hello $world\"\n",
        "echo 'single'\n",
        "x=$((1 + 2))\n",
        "x=$(( (a % (b - c + 1)) + c ))\n",
        "echo $(date) `whoami` ${HOME}/x\n",
        "echo \"nested $(echo \"$(echo inner)\")\"\n",
        "x=$(if true; then echo yes; fi)\n",
        "x=$(for i in 1 2; do echo $i; done)\n",
        "x=`if true; then echo yes; fi`\n",
        "echo ${arr[@]} ${arr[0]}\n",
        "echo ${x%%:*} ${x##*:} ${x:-y} ${x-cmp} ${x:1:3} ${x/a/b}\n",
        "echo ${ip_addrs//,/ }\n",
        "x=$'a\\nb'\n",
        "x=$'it\\'s'\n",
        "echo pre$'x'post\n",
        "if (( ${#arr[@]} != 1 )); then :; fi\n",
        "if (( \"${#PACMAN_COLOR[@]}\" )); then :; fi\n",
        "x=$(( (a % (b - c + 1)) + c ))\n",
        "x=$(( ((a)) + 1 ))\n",
        "x=$(( $(date +%s) + 30 ))\n",
        "lvremove_deadline=\"$(( $(date \"+%s\") + 30))\"\n",
        "echo ${path/${JVM_DIR}\\/}\n",
        "[[ $1 =~ kernel_(preinst|postinst) ]]\n",
        "[[ \"$x\" =~ ^[0-9]+$ ]]\n",
        "[[ $x =~ [[:space:]] ]]\n",
        "echo Magick++-7.Q16HDRI\n",
        "echo a<=b\n",
        "arr=(one two three)\n",
        "declare -A map\nmap[key]=value\n",
        "eval argv$n=\\$1\n",
        "x=`echo a | sed 's/x//'`\n",
        "n=`expr $n + 1`\n",
        "`expr 1 + 2 * 3`\n",
        "find . -exec ls {} +\n",
        "maxps=$(($(printf \"%4i*%s\\n\" $((maxps_hex & 0x7ff)) $((1 + ($((maxps_hex >> 11)) & 0x3))))))\n",
        "echo Bokmål\n",
        "Bokmål=123\n",
        "find . -name '*.txt' -exec ls {} +\n",
        "[[ -n \"$x\" && \"$x\" == \"y\" ]]\n",
        "(( i++ ))\n",
        "d=\"${d//+([!\\/])/..}\"\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_heredocs) {
    accepts({
        "cat <<EOF\nhello $name\nEOF\n",
        "cat <<-EOF\n\tindented\n\tEOF\n",
        "cat << _EOF_\nbody\n_EOF_\n",
        "cat <<A <<B\nfirst\nA\nsecond\nB\necho after\n",
        "cat <<EOF | grep x\nbody\nEOF\necho after\n",
        "usage() {\n    cat <<EOF\n${2:+$2\n\n}Usage: x\nEOF\n    exit 1\n}\n",
        "x=$(cat <<EOF\nbody\nEOF\n)\n",
        "x=\"$(cat <<EOF\nbody\nEOF\n)\"\n",
        "x=$(cat <<-EOF\n\tbody\n\tEOF\n)\n",
        "x=$(cat <<'EOF'\n$body\nEOF\n)\n",
        "x=$(cat <<EOF | grep b\nbody\nEOF\n)\n",
        "x=$(echo a; cat <<EOF\nbody\nEOF\n)\n",
        "x=$(cat <<EOF\nbody\nEOF\necho b\n)\n",
        "x=$(echo $(cat <<EOF\nbody\nEOF\n))\n",
        "x=$(f() { cat <<EOF\nbody\nEOF\n}; f)\n",
        "x=$( (cat <<EOF\nbody\nEOF\n) )\n",
        "x=$(if cat <<EOF >/dev/null\nbody\nEOF\nthen echo yes; fi)\n",
        "x=$(cat <<A <<B\nfirst\nA\nsecond\nB\n)\n",
        "cat <<EOF\nhello\nEOF\n\nexit 1\n",
        "f() {\n    cat <<EOF\nhello\nEOF\n\n    exit 1\n}\n",
    });
}

TEST_F(BashFullGrammarFixture, parses_comments_and_continuations) {
    accepts({
        "# just a comment\n",
        "echo hi # trailing\n",
        "echo a \\\n  b \\\n  c\n",
        "printf \"a\" \\\n  \"b\"\n",
        "f() {\n  printf \"a\" \\\n    \"b\"\n}\n",
    });
}

TEST_F(BashFullGrammarFixture, rejects_incomplete_control_flow) {
    EXPECT_FALSE(parses("if true; then echo yes\n"));
    EXPECT_FALSE(parses("for i in 1 2 3; do echo $i\n"));
    EXPECT_FALSE(parses("while true; do echo a\n"));
    EXPECT_FALSE(parses("case x in a) echo a\n"));
    EXPECT_FALSE(parses("echo \"unterminated\n"));
    EXPECT_FALSE(parses("cat <<EOF\nbody\n"));
}

TEST_F(BashFullGrammarFixture, parses_reserved_words_as_arguments) {
    accepts({
        "echo done\n",
        "echo if then else elif fi\n",
        "echo for in do done while until case select function\n",
        "git checkout -b for\n",
        "help if\n",
        "while true; do echo done; done\n",
        "if true; then echo fi; fi\n",
        "for i in done fi; do echo $i; done\n",
        "x=done y=if z=1\n",
        "echo a > done\n",
    });
}

TEST_F(BashFullGrammarFixture, rejects_isolated_reserved_words_in_command_position) {
    EXPECT_FALSE(parses("done\n"));
    EXPECT_FALSE(parses("fi\n"));
    EXPECT_FALSE(parses("then\n"));
    EXPECT_FALSE(parses("esac\n"));
    EXPECT_FALSE(parses("elif\n"));
}

} // namespace
