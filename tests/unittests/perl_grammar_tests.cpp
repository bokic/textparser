#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <perl_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>
#include <vector>

namespace {

struct PerlGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/perl_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &perl_definition;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, PerlGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(PerlGrammarFixture, declarations_and_modules) {
    for (const char *source : {
        "", "# comment\n", "#!/usr/bin/perl\nuse strict; use warnings;",
        "package Foo::Bar 1.23; use Exporter qw(import); our @EXPORT = qw(foo); 1;",
        "package Foo { our $VERSION = '1.0'; sub foo { return 1 } }",
        "my $x = 1; our @a = (1,2,3); local $/ = undef; state %h = (a => 1);",
        "my ($a, @rest) = @_; my $x = 1 + 2 * 3; $x += 2;",
        "sub foo; sub bar ($$;@) { return $_[0] + $_[1]; }",
        "sub greet ($name, $suffix = '!') { return $name . $suffix; }",
        "sub foo :lvalue { $x } my $f = sub ($x) { $x + 1 };",
        "BEGIN { $x = 1; } CHECK { $x++; } END { print $x; }",
        "class Point { field $x :param = 0; method x () { $x } ADJUST { $x++ } }",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, control_flow) {
    for (const char *source : {
        "if ($x) { print 'yes'; } elsif ($y) { print 'maybe'; } else { die 'no'; }",
        "unless ($x) { $x = 1 } while ($x < 10) { ++$x; } continue { print $x; }",
        "for (my $i = 0; $i < 10; $i++) { next if $i == 5; }",
        "foreach my $item (@items) { print $item; }",
        "OUTER: for (;;) { last OUTER; }",
        "do { $x++ } until $x > 10; print $_ for @items;",
        "given ($x) { when (1) { say 'one'; } default { say 'other'; } }",
        "try { foo(); } catch ($e) { warn $e; } finally { cleanup(); }",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, expressions_and_contextual_tokens) {
    for (const char *source : {
        "my $x = -2 ** 2 + 3 * 4; $x = $y = 1; $x ||= 2; $x //= 3;",
        "my $x = $a ? $b : $c; $x = 1 .. 10; print $x or die 'failed';",
        "my $a = [1, 2, 3]; my $h = { name => 'Joe', count => 3 };",
        "$obj->method(1, 2)->{name}; $a[0] = $h{key}; $ref->[0]->foo;",
        "my $ref = \\%hash; my @a = @$ref; my $x = ${$ref};",
        "my @a = $ref->@*; my %h = $ref->%*;",
        "my @a = map { $_ * 2 } @values; my @b = grep { $_ > 0 } @a;",
        "my $x = length $s + 1; print join ',', @a; open($fh, '<', $path) or die $!;",
        "my $x = 10 / 2 / 5; my $y = 2*foo(); my $z = 5%foo();",
        "my $x = 0xff + 0b101 + 0o77 + .5e2; my $v = v5.42.0;",
        "my $x = $Pkg::name; $_ = $1 . $^O . $!; @ARGV = ();",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, quoting_and_regular_expressions) {
    for (const char *source : {
        "my $s = q{a {nested} string}; my @a = qw(one two three);",
        "my $s = qq!hello $name!; my $cmd = qx(echo hello);",
        "my $re = qr{a{2}}i; $s =~ /foo[\\/]bar/i; $s !~ m!no!;",
        "$s =~ s/foo/bar/g; $s =~ s{foo}{bar}g; $s =~ tr/a-z/A-Z/;",
        "$s =~ s{a{2}}(b)g; my $s = q XhelloX;",
        "my $s = 'multi\nline'; my $s2 = \"escaped \\\" quote\";",
        "my $line = <STDIN>; while (my $line = <$fh>) { print $line; }",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, heredocs_pod_and_data) {
    for (const char *source : {
        "my $s = <<'END';\nhello\nEND\nprint $s;",
        "my $s = <<'';\nhello\n\nprint $s;",
        "my $s = <<\"\";\nworld\n\nprint $s;",
        "my $s = <<~'END';\n    hello\n    END\nprint $s;",
        "my $s = <<~\"END\";\n    hello\n    END\nprint $s;",
        "my $s = <<`CMD`;\necho hello\nCMD\nprint $s;",
        "my ($a, $b) = (<<'A', <<\"B\");\none\nA\ntwo\nB\nprint $a, $b;",
        "my $s = <<END;\nhello\nEND\n",
        "=pod\nDocumentation\n=cut\nprint 1;",
        "print 1;\n__DATA__\nnot perl {{{",
        "format STDOUT =\n@<<<<\n$value\n.\nwrite;",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, rejects_incomplete_and_malformed_sources) {
    for (const char *source : {
        "my $x = ;", "if ($x) {", "sub foo {", "my @a = [1,2;",
        "my $x = 'unterminated", "my $x = q{unclosed;", "my $x = /unclosed;",
        "my $x = <<'END';\nbody\n", "my $x = <<'END';",
        "my $x = 1; }", "my $x = 1; \x01",
    }) parse_source(source, TEXTPARSER_MATCH_NO);
}

TEST_P(PerlGrammarFixture, prototypes_control_call_precedence) {
    auto *root = parse_source("sub unary ($); my $x = unary 1 + 2 == 3;");
    auto *call = find(root, "PrototypedUnaryCall");
    ASSERT_NE(call, nullptr);
    EXPECT_NE(find(call, "Plus"), nullptr);
    EXPECT_EQ(find(call, "Equal"), nullptr);
    EXPECT_NE(find(root, "Equal"), nullptr);

    root = parse_source("sub nullary (); my $x = nullary + 1;");
    call = find(root, "NullaryCall");
    ASSERT_NE(call, nullptr);
    EXPECT_EQ(find(call, "Plus"), nullptr);
    EXPECT_NE(find(root, "Plus"), nullptr);

    root = parse_source("sub list ($;); my @x = list 1 + 2, 3;");
    call = find(root, "DeclaredListCall");
    ASSERT_NE(call, nullptr);
    EXPECT_NE(find(call, "Comma"), nullptr);
    parse_source("sub apply (&@); apply { $_ * 2 } @values;");
    parse_source("sub unary ($); unary;", TEXTPARSER_MATCH_NO);
    parse_source("sub unary ($); unary 2;");
    // Re-execution rebuilds declarations; cached matches must not skip them.
    textparser_match_result again = {};
    ASSERT_EQ(parser.execute_language_grammar(get_definition(), &again), 0);
    EXPECT_EQ(again.status, TEXTPARSER_MATCH_OK);
    EXPECT_NE(find(again.node, "PrototypedUnaryCall"), nullptr);
}

TEST_P(PerlGrammarFixture, verifies_operator_tree_shape) {
    auto *root = parse_source("my $x = -2 ** 2 + 3 * 4;");
    auto *sum = find(root, "Plus");
    ASSERT_NE(sum, nullptr);
    EXPECT_NE(find(sum, "Star"), nullptr);
    auto *minus = find(sum, "Minus");
    ASSERT_NE(minus, nullptr);
    EXPECT_NE(find(minus, "Power"), nullptr);
    root = parse_source("my $x = 1; $x = $y = 2;");
    ASSERT_NE(find(root, "Assign"), nullptr);
}

TEST_P(PerlGrammarFixture, call_and_dereference_edges) {
    for (const char *source : {
        "print STDERR 'error'; print $fh \"hello\"; print {$fh} $text;",
        "my @a = (1, 2,); my $h = { a => 1, }; my $a = [1, 2,];",
        "my $x = ${^UNICODE}; my $y = $$ref; my @a = @$ref; my %h = %$ref;",
        "my $count = $#{$ref}; &foo(1, 2);",
        "sub has; has name => (is => 'ro', required => 1);",
        "sub new { my ($class, %args) = @_; return bless \\%args, $class; }",
        "my $s = <<'E'\nbody\nE\n;",
        "my $s = (<<'E'\nbody\nE\n);",
        "my $s = <<~'E';\r\n \tbody\r\n \tE\r\nprint $s;",
    }) parse_source(source);
}

TEST_P(PerlGrammarFixture, package_and_lexical_prototype_scopes) {
    auto *root = parse_source("package A; sub f (); package B; sub f ($); my $x = f 2 + 3; A::f + 1;");
    EXPECT_NE(find(root, "PrototypedUnaryCall"), nullptr);
    EXPECT_NE(find(root, "NullaryCall"), nullptr);
    root = parse_source("sub f (); { my sub f ($); f 1 + 2; } f + 1;");
    EXPECT_NE(find(root, "PrototypedUnaryCall"), nullptr);
    EXPECT_NE(find(root, "NullaryCall"), nullptr);
    root = parse_source("package A; sub f (); { package B; sub f ($); f 1; } f + 1;");
    EXPECT_NE(find(root, "PrototypedUnaryCall"), nullptr);
    EXPECT_NE(find(root, "NullaryCall"), nullptr);
    root = parse_source("sub f (); package B { sub f ($); f 1; } main::f + 1;");
    EXPECT_NE(find(root, "NullaryCall"), nullptr);
}

TEST_P(PerlGrammarFixture, lexer_lookahead_and_speculation_preserve_heredocs) {
    const char *source = "my $s = <<'';\nbody\n\nprint 1;";
    parser.reset();
    ASSERT_EQ(parser.openmem(source, std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(parser.parse(get_definition()), 0);
    // An empty production initializes the grammar cursor at source offset zero.
    textparser_production empty = {};
    empty.kind = TEXTPARSER_PROD_SEQUENCE;
    ASSERT_EQ(parser.execute_production(&empty, 1, 0, &result), 0);
    textparser_clear_diagnostics(parser.get());
    // Looking arbitrarily far ahead must not register or drain captures.
    for (size_t ahead : {3u, 6u, 9u, 50u}) {
        const textparser_lex_token *token = nullptr;
        EXPECT_GE(textparser_lexer_peek(parser.get(), ahead, "ExpressionStart", &token), 0);
    }
    void *outer = nullptr;
    textparser_speculate_begin(parser.get(), &outer);
    ASSERT_NE(outer, nullptr);
    const textparser_lex_token *token = nullptr;
    for (int i = 0; i < 5; ++i)
        ASSERT_EQ(textparser_lexer_consume(parser.get(), "ExpressionStart", &token), 0);
    void *inner = nullptr;
    textparser_speculate_begin(parser.get(), &inner);
    ASSERT_NE(inner, nullptr);
    std::vector<int> first;
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(textparser_lexer_consume(parser.get(), "ExpressionStart", &token), 0);
        first.push_back(token->kind);
    }
    textparser_speculate_rollback(parser.get(), inner);
    for (int kind : first) {
        ASSERT_EQ(textparser_lexer_consume(parser.get(), "ExpressionStart", &token), 0);
        EXPECT_EQ(token->kind, kind);
    }
    textparser_speculate_rollback(parser.get(), outer);
    ASSERT_EQ(parser.execute_language_grammar(get_definition(), &result), 0);
    EXPECT_EQ(result.status, TEXTPARSER_MATCH_OK);
    EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
}

TEST_P(PerlGrammarFixture, recovery_retains_following_declarations) {
    auto *root = parse_source("my $broken = ; sub retained { return 42; }", TEXTPARSER_MATCH_OK, true);
    EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(root, "SubDeclaration"), nullptr);
}

TEST_P(PerlGrammarFixture, line_boundaries_indentation_and_shift) {
    for (const char *source : {
        "my $s = <<~'E';\n  body\n\n  E\nprint 1;",
        "my ($a, $b) = (<<~'A', <<~'B');\n  one\n  A\n    two\n    B\n",
        "my $x = 1 << FOO; my $y = 3 << 2;",
        "=pod\nunterminated documentation is legal\n",
        "my $x = 1;\n__END__\nignored data\n",
        "my $x = 1; __END__\nignored data\n",
    }) parse_source(source);
    for (const char *source : {
        "my $s = <<~'E';\nbody\n  E\n",
        "my $s = <<~'E';\n \tbody\n\t E\n",
        "my $s = <<'E';\nbody\n E\n",
        "my $x = 1; =pod\nnot a POD block here\n=cut\n",
    }) parse_source(source, TEXTPARSER_MATCH_NO);
}
} // namespace
