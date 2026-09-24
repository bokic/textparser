#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <scratch_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct ScratchGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/scratch_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
            ASSERT_NE(json_definition->grammar, nullptr);
        }
    }

    void TearDown() override {
        if (json_definition != nullptr) {
            textparser_free_language_definition(json_definition);
            json_definition = nullptr;
        }
    }

    const textparser_language_definition *get_definition() const {
        return GetParam() ? json_definition : &scratch_definition;
    }

    textparser_node *parse_source(const char *source,
                                  textparser_match_status expected = TEXTPARSER_MATCH_OK,
                                  bool allow_diagnostics = false) {
        SCOPED_TRACE(source);
        parser.reset();
        EXPECT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        EXPECT_EQ(parser.parse(get_definition()), 0);
        result = {};
        EXPECT_EQ(parser.execute_language_grammar(get_definition(), &result), 0);
        if (result.status == TEXTPARSER_MATCH_OK) {
            const textparser_lex_token *remaining = nullptr;
            int peek = textparser_lexer_peek(
                parser.get(), 0, textparser_get_lexical_goal(parser.get()), &remaining);
            if (peek == 0 && remaining != nullptr) {
                result = {};
                result.status = TEXTPARSER_MATCH_NO;
            }
        }
        if (expected == TEXTPARSER_MATCH_NO &&
            (result.status == TEXTPARSER_MATCH_ERROR || textparser_get_diagnostic_count(parser.get()) != 0)) {
            result.status = TEXTPARSER_MATCH_NO;
        }
        if (textparser_get_diagnostic_count(parser.get()) != 0) {
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
        }
        if (expected == TEXTPARSER_MATCH_OK && !allow_diagnostics) {
            EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u) << source;
        }
        if (result.status != expected) {
            std::cout << "Parse failed for source:\n" << source << "\nStatus: " << result.status << std::endl;
            const textparser_lex_token *rem = nullptr;
            textparser_lexer_peek(parser.get(), 0, textparser_get_lexical_goal(parser.get()), &rem);
            if (rem) {
                std::string token_str = (rem->end <= std::strlen(source)) ? std::string(source + rem->start, rem->end - rem->start) : "";
                std::cout << "Remaining token: kind=" << rem->kind << ", text='" << token_str << "', span=[" << rem->start << ", " << rem->end << "]" << std::endl;
            }
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
        }
        EXPECT_EQ(result.status, expected) << source;
        return result.node;
    }

    textparser::Parser parser;
    textparser_match_result result = {};
};

INSTANTIATE_TEST_SUITE_P(DefinitionSources,
                         ScratchGrammarFixture,
                         testing::Values(false, true),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(ScratchGrammarFixture, parses_hat_blocks_and_events) {
    const char *source = R"(
when green flag clicked
    move 10 steps

when flag clicked
    show

when this sprite clicked
    broadcast message1

when stage clicked
    next backdrop

when space key pressed
    turn cw 15 degrees

when [up arrow v] key pressed
    change y by 10

when backdrop switches to [backdrop2 v]
    set size to 100

when loudness > 50
    say [Too loud!] for 2 seconds

when timer > 10
    stop all

when receive [game over v]
    hide

when I receive message1
    start sound meow

when I start as a clone
    go to x: 0 y: 0
    show

define jump (height)
    change y by height
    wait 0.2 seconds
    change y by (0 - height)
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(textparser_node_get_category(root), TEXTPARSER_CST_SOURCE_FILE);
}

TEST_P(ScratchGrammarFixture, parses_motion_blocks) {
    const char *source = R"(
move 10 steps
move (20) steps
move (-15) steps
turn cw 15 degrees
turn cw (45) degrees
turn right 90 degrees
turn ccw 15 degrees
turn left 90 degrees
point in direction 90
point in direction (-90)
point towards (mouse-pointer)
point towards [mouse-pointer v]
go to x: 0 y: 0
go to x: (100) y: (-50)
go to (mouse-pointer)
go to (random position)
glide 1 secs to x: 0 y: 0
glide (2) seconds to x: (50) y: (50)
glide 1 secs to (random position)
change x by 10
change x by (-10)
set x to 0
set x to (100)
change y by 5
set y to (-50)
if on edge, bounce
set rotation style [left-right v]
set rotation style [don't rotate v]
set rotation style [all around v]
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(ScratchGrammarFixture, parses_looks_and_sound_blocks) {
    const char *source = R"(
say [Hello!] for 2 seconds
say Hello for 2 seconds
say (score) for (1) secs
say [Hello!]
say Hello
think [Hmm...] for 2 seconds
think [Hmm...]
switch costume to [costume1 v]
switch costume to costume2
next costume
switch backdrop to [backdrop1 v]
next backdrop
change size by 10
change size by (-10)
set size to 100
set size to (150)
change [color v] effect by 25
set [ghost v] effect to 50
clear graphic effects
clear
show
hide
go to [front v] layer
go to [back v] layer
go forward 1 layers
go backward 2 layers
play sound [Meow v] until done
start sound [Meow v]
play sound meow
stop all sounds
clear sound effects
change volume by (-10)
set volume to 100
set volume to (80)
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(ScratchGrammarFixture, parses_control_and_sensing_blocks) {
    const char *source = R"(
wait 1 seconds
wait (0.5) secs
wait 2
wait until <touching (mouse-pointer)>

repeat 10
    move 10 steps
end

forever
    next costume
    wait 0.1 seconds
end

if <(x) > (100)> then
    set x to 0
end

if <touching (mouse-pointer)> then
    say [Found it!]
else
    move 5 steps
end

repeat until <(lives) = (0)>
    move 10 steps
    if on edge, bounce
end

stop all
stop this script
stop other scripts in sprite

create clone of (myself)
create clone of [myself v]
delete this clone

ask [What is your name?] and wait
reset timer
set drag mode [draggable v]
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(ScratchGrammarFixture, parses_variables_and_lists) {
    const char *source = R"(
set [score v] to 0
set [score v] to (score + 1)
set score to 100
set x to 50
set y to 25
change [score v] by 1
change score by (-1)
show variable [score v]
hide variable [score v]
add [apple] to [inventory v]
add (item_name) to [inventory v]
delete 1 of [inventory v]
delete (1) of [inventory v]
delete all of [inventory v]
insert [banana] at 1 of [inventory v]
replace item 1 of [inventory v] with [orange]
show list [inventory v]
hide list [inventory v]
x = -1
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(ScratchGrammarFixture, parses_expressions_and_operators) {
    const char *source = R"(
set val to (10 + 20)
set val to (10 - 5)
set val to (4 * 5)
set val to (100 / 2)
set val to (17 mod 5)
set val to ((10 + 20) * (30 - 5))
set val to pick random 1 to 10
set val to pick random (1) to (100)
set val to join [apple ] [banana]
set val to letter 1 of [apple]
set val to length of [apple]
set val to round 3.14
set val to round (x)
set val to sqrt of 16
set val to abs of (-5)
set val to floor of 3.9
set val to ceiling of 3.1
set val to sin of 90
set val to cos of 0
set val to item 1 of [list v]
if <(a) < (b)> then
    show
end
if <(a) <= (b)> then
    show
end
if <(a) = (b)> then
    show
end
if <(a) != (b)> then
    show
end
if <(a) > (b)> then
    show
end
if <(a) >= (b)> then
    show
end
if <<(a) = (1)> and <(b) = (2)>> then
    show
end
if <<(a) = (1)> or <(b) = (2)>> then
    show
end
if <not <touching (mouse-pointer)>> then
    show
end
set d to distance to (mouse-pointer)
set ans to answer
set mx to mouse x
set my to mouse y
set t to timer
set loud to loudness
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
}

TEST_P(ScratchGrammarFixture, parses_complete_sample_program) {
    const char *source = R"(
# Scratch cat animation
when green flag clicked
set size to (100)
show
forever
    move (10) steps
    if touching (mouse-pointer) then
        say Hello for 2 seconds
    else
        move (-10) steps
    end
    wait 0.5 seconds
end

when this sprite clicked
broadcast message1
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(textparser_node_get_category(root), TEXTPARSER_CST_SOURCE_FILE);
}

TEST_P(ScratchGrammarFixture, recovers_from_syntax_errors) {
    const char *source = R"(
when green flag clicked
    move 10 steps
    @#$% completely invalid block statement here ;
    show
)";
    textparser_node *root = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(root, nullptr);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
}

TEST_P(ScratchGrammarFixture, rejects_invalid_syntax) {
    const char *source = "@#$%^&";
    parse_source(source, TEXTPARSER_MATCH_NO, true);
}

} // namespace
