#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <php.h>
#include <php_definition.json.h>

#include <cstring>
#include <string>

namespace {

struct PHPGrammarFixture : testing::TestWithParam<bool> {
    const textparser_language_definition *definition = nullptr;
    textparser_language_definition *owned_definition = nullptr;
    textparser::Parser parser;
    textparser_match_result result{};

    void SetUp() override {
        if (GetParam()) {
            definition = &php_definition;
        } else {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/php_definition.json", &owned_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            definition = owned_definition;
        }
        ASSERT_NE(definition, nullptr);
        ASSERT_NE(definition->grammar, nullptr);
        EXPECT_DOUBLE_EQ(definition->version, 8.5);
    }

    void TearDown() override {
        parser.reset();
        if (owned_definition) textparser_free_language_definition(owned_definition);
    }

    void parse(const char *source) {
        parser.reset();
        ASSERT_EQ(parser.openmem(source, (int)std::strlen(source), TEXTPARSER_ENCODING_UTF_8), 0);
        ASSERT_EQ(textparser_php_register_validators(parser.get()), 0);
        ASSERT_EQ(parser.parse(definition), 0);
        result = {};
        ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    }

    void accepts(const char *source, bool trailing_trivia = false) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        ASSERT_EQ(result.status, TEXTPARSER_MATCH_OK);
        EXPECT_EQ(textparser_get_diagnostic_count(parser.get()), 0u);
        if (!trailing_trivia) {
            ASSERT_NE(result.node, nullptr);
            // Checking status alone missed the draft stopping immediately after <?php.
            EXPECT_EQ(result.node->source_end, std::strlen(source));
        }
        const textparser_lex_token *remaining = nullptr;
        EXPECT_EQ(textparser_lexer_peek(parser.get(), 0,
                      textparser_get_lexical_goal(parser.get()), &remaining), 1);
        EXPECT_EQ(remaining, nullptr);
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

    void rejects(const char *source, const char *code) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        bool found = false;
        for (size_t i = 0; i < textparser_get_diagnostic_count(parser.get()); ++i) {
            textparser_diagnostic diagnostic{};
            ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
            if (std::strcmp(diagnostic.code, code) == 0) {
                found = true;
                EXPECT_EQ(diagnostic.severity, TEXTPARSER_SEVERITY_ERROR);
                EXPECT_GT(diagnostic.length, 0u);
                EXPECT_LE(diagnostic.start_pos + diagnostic.length, std::strlen(source));
            }
        }
        EXPECT_TRUE(found) << code;
    }
};

TEST_P(PHPGrammarFixture, WhitespaceAndCommentsDoNotTruncateSource) {
    for (const char *source : {
             "<?php echo \"hello\"; ?>",
             "<?php\t\r\n$x = 1;\r\n?>",
             "<?php /* block\ncomment */ // line\n# shell\n$x = 1; ?>",
             "<?php $x /* between operands */ = /* value */ 1; ?>",
             "<?php /* only trivia */ ?>",
             "<?php ?>",
             "<?php $x=1;"}) {
        accepts(source);
    }
}

TEST_P(PHPGrammarFixture, StatementsAndPostfixExpressions) {
    for (const char *source : {
             "<?php function add($a, $b) { return $a + $b; } add(1,2); ?>",
             "<?php $a = [1, 2, 'key' => 3]; $obj->method($a[0]); ?>",
             "<?php if (true) { echo 'yes'; } else { echo 'no'; } ?>",
             "<?php $x = []; $obj?->method(); Foo::bar(); ?>",
             "<?php $s = '// not a comment'; $t = \"/* text */\"; ?>"}) {
        accepts(source);
    }
}

TEST_P(PHPGrammarFixture, AssignmentTokensAndArithmeticPrecedence) {
    accepts("<?php $x = 2 + 3 * 4; $y += 1; ?>");
    auto *assignment = find(result.node, "AssignOperator");
    ASSERT_NE(assignment, nullptr);
    ASSERT_NE(assignment->child, nullptr);
    auto *sum = assignment->child->next;
    ASSERT_NE(sum, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), sum), "AddOperator");
    ASSERT_NE(sum->child, nullptr);
    ASSERT_NE(sum->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), sum->child->next), "MulOperator");
    EXPECT_NE(find(result.node, "CompoundAssignOperator"), nullptr);
}

TEST_P(PHPGrammarFixture, AssignmentIsRightAssociative) {
    accepts("<?php $a = $b = 1; ?>");
    auto *assignment = find(result.node, "AssignOperator");
    ASSERT_NE(assignment, nullptr);
    ASSERT_NE(assignment->child, nullptr);
    ASSERT_NE(assignment->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), assignment->child->next),
                 "AssignOperator");
}

TEST_P(PHPGrammarFixture, MissingAssignmentOperandReportsDiagnostic) {
    ASSERT_NO_FATAL_FAILURE(parse("<?php $x = ; ?>"));
    EXPECT_NE(result.status, TEXTPARSER_MATCH_OK);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "PHP1001");
}

TEST_P(PHPGrammarFixture, OutsideTextIsTrivia) {
    for (const char *source : {
             "", "arbitrary $x = ; {} <b>text</b> ?>\n",
             "before <?php echo 1; ?> after",
             "<b><?php echo 1 ?></b>",
             "<!-- <?php echo 1; ?> -->",
             "prefix <?php ?> middle <?php ?> suffix",
             "prefix <? echo 1 ?> suffix",
             "outside < ?php is text < trailing"}) {
        accepts(source, true);
        EXPECT_EQ(find(result.node, "OutsidePHP"), nullptr);
        EXPECT_EQ(find(result.node, "HtmlTag"), nullptr);
        EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
    }
    int trivia_id = -1;
    for (int i = 0; definition->tokens[i].name; ++i) {
        if (std::strcmp(definition->tokens[i].name, "OutsidePHP") == 0) trivia_id = i;
    }
    ASSERT_GE(trivia_id, 0);
    EXPECT_TRUE(definition->lexer_rules[trivia_id].is_trivia);
}

TEST_P(PHPGrammarFixture, TransitionsWithinBodiesAndShortEcho) {
    for (const char *source : {
             "<?php if (true) { ?>text<?php echo 1 ?>more<?php } ?>tail",
             "<?php function render() { ?>html<?php return; } ?>",
             "a<?= 1 + 2 ?>b<?= 'end'; ?>c",
             "<?php echo 1 ?><?php echo 2 ?>",
             "<?php $x = 1 ?>after",
             "<?php if (true) { ?><?= 1 ?><?php } ?>"}) {
        accepts(source, true);
        EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
    }
}

TEST_P(PHPGrammarFixture, CommentAndStringTagBoundaries) {
    for (const char *source : {
             "<?php echo 1; // comment ?>after",
             "<?php echo 1 # comment ?>after",
             "<?php # comment with ] and [ ?>after",
             "<?php // question ? not close\r\necho 1 ?>after",
             "<?php /* ?> <?php */ echo 1 ?>after",
             "<?php echo '<?php ?>', \"?> <?php\"; ?>after"}) {
        accepts(source, true);
        EXPECT_NE(find(result.node, "PHPCloseTag"), nullptr);
        EXPECT_STREQ(textparser_get_current_mode(parser.get()), "default");
    }
    accepts("<?php echo 1; // no closing tag", true);
    EXPECT_STREQ(textparser_get_current_mode(parser.get()), "PHP");
}

TEST_P(PHPGrammarFixture, OutsideTriviaDoesNotHideErrorsInsidePHP) {
    for (const char *source : {
             "before <?php $x = ; ?> after",
             "before <?= ?> after",
             "before <?php echo ; ?> after",
             "before <?php echo 1 ?> middle <?php $x = ; ?> after"}) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        EXPECT_TRUE(result.status != TEXTPARSER_MATCH_OK ||
                    textparser_get_diagnostic_count(parser.get()) > 0);
    }
}

TEST_P(PHPGrammarFixture, PHP85SyntaxCorpus) {
    struct Sample { const char *name; const char *body; };
    const Sample samples[] = {
        {"pipe", "$x = \"hello\" |> strlen(...);"},
        {"void", "(void) strlen(\"hello\");"},
        {"hooks", "class C { public string $name { get => $this->name; set { $this->name = $value; } } }"},
        {"asymmetric", "class C { public private(set) string $name; public static protected(set) int $count = 0; }"},
        {"promoted", "class C { public function __construct(public final string $name) {} }"},
        {"dnf", "function f((A&B)|C $x): (A&B)|null { return null; }"},
        {"byref", "function f(A &$x) {}"},
        {"constants", "#[Deprecated] const X = 1; class C { public const string X = \"a\"; }"},
        {"use", "use Foo\\Bar\\{Baz, function qux, const X};"},
        {"anonymous", "$x = new class(1) extends Base implements I { public function f(): void {} };"},
        {"alt", "if ($x): echo 1; elseif ($y): echo 2; else: echo 3; endif;"},
        {"switch", "switch ($x) { case 1: echo 1; break; default: echo 2; }"},
        {"altswitch", "switch ($x): case 1: break; default: break; endswitch;"},
        {"loops", "while ($x): break; endwhile; for (;;): break; endfor; foreach ($x as $v): echo $v; endforeach;"},
        {"declare", "declare(strict_types=1); declare(ticks=1): echo 1; enddeclare;"},
        {"extras", "static $x=1; goto end; end: unset($x);"},
        {"catch", "try { work(); } catch (A|B $e) {} finally {}"},
        {"legacyarray", "$x = array(1, \"key\" => &$v); list($a, $b) = $x;"},
        {"yield", "function gen() { yield; yield 1; yield \"key\" => 2; yield from $x; }"},
        {"dynamic", "$$name = ${\"x\"}; $obj->{$name}();"},
        {"clone", "$x = clone($obj, [\"name\" => \"new\"]);"},
        {"keyword", "class C { function match() {} } $x->match();"},
        {"heredoc", "$x = <<<END\nhello\nEND;"},
        {"nowdoc", "$x = <<<'END'\nhello\nEND;"},
        {"interp", "$x = \"hello {$user->name}\";"},
        {"expressions", "$x = $a ? $b : $c ?? throw new Exception(); $y = (int) 1.2; $z = 2 ** 3 ** 2;"},
        {"emptyheredoc", "$x = <<<END\nEND;"},
        {"attrs", "#[A] function f(#[B] $x) {} #[C] enum E { #[D] case A; }"},
        {"reference", "$x = &$y;"},
        {"arrayholes", "[, $x] = $a;"},
        {"firstclass", "$f = Foo::method(...);"},
    };
    for (const auto &sample : samples) {
        SCOPED_TRACE(sample.name);
        const std::string source = std::string("<?php ") + sample.body + " ?>";
        accepts(source.c_str());
    }
}

TEST_P(PHPGrammarFixture, PHP85MalformedConstructsProduceDiagnostics) {
    for (const char *source : {
             "<?php $x = 1 |> ; ?>",
             "<?php $x = (void) f(); ?>",
             "<?php function f(A| $x) {} ?>",
             "<?php class C { public const X = ; } ?>",
             "<?php class C { public string $x { nope => 1; } } ?>",
             "<?php try {} ?>",
             "<?php if (true): echo 1; ?>"}) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    }
}

TEST_P(PHPGrammarFixture, PHP85PrecedenceTreeShapes) {
    accepts("<?php $x = 'n=' . 1 + 2; ?>");
    auto *concat = find(result.node, "ConcatOperator");
    ASSERT_NE(concat, nullptr);
    ASSERT_NE(concat->child, nullptr);
    ASSERT_NE(concat->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), concat->child->next), "AddOperator");

    accepts("<?php $x = 1 + 2 |> transform(...); ?>");
    auto *pipe = find(result.node, "PipeOperator");
    ASSERT_NE(pipe, nullptr);
    ASSERT_NE(pipe->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), pipe->child), "AddOperator");

    accepts("<?php $x = -2 ** 2; ?>");
    auto *unary = find(result.node, "AddOperator");
    ASSERT_NE(unary, nullptr);
    ASSERT_NE(unary->child, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), unary->child), "PowerOperator");

    accepts("<?php $x = $a ?? $b ?? $c; ?>");
    auto *coalesce = find(result.node, "NullCoalescingOperator");
    ASSERT_NE(coalesce, nullptr);
    ASSERT_NE(coalesce->child, nullptr);
    ASSERT_NE(coalesce->child->next, nullptr);
    EXPECT_STREQ(textparser_grammar_node_name(parser.get(), coalesce->child->next),
                 "NullCoalescingOperator");
}

TEST_P(PHPGrammarFixture, ValidatesAssignmentTargets) {
    for (const char *source : {
             "<?php 1 = 2; ?>", "<?php f() = 2; ?>", "<?php ($a) = 2; ?>",
             "<?php [1, $a] = $v; ?>", "<?php $a?->x = 2; ?>",
             "<?php $a?->x[0] = 2; ?>", "<?php Foo::X = 2; ?>",
             "<?php $this = 2; ?>", "<?php true += 2; ?>"})
        rejects(source, "PHP2001");
    for (const char *source : {
             "<?php $a = 1; $a[0] = 2; $a[] = 3; ?>",
             "<?php f()[0] = 2; f()->x = 3; ?>",
             "<?php $this->x = 1; Foo::$x = 2; $$name = 3; ?>",
             "<?php [$a, , $b] = $v; ['x' => $a] = $v; list($a) = $v; ?>",
             "<?php $a[$b?->key] = 1; ?>",
             "<?php ($a)->x = 1; (f())->x = 2; ?>"})
        accepts(source);
}

TEST_P(PHPGrammarFixture, ValidatesUpdateTargets) {
    for (const char *source : {
             "<?php ++1; ?>", "<?php false--; ?>", "<?php f()++; ?>",
             "<?php ++($a); ?>", "<?php $a?->x++; ?>"})
        rejects(source, "PHP2002");
    accepts("<?php ++$a; $a[0]--; --Foo::$x; $this->x++; ?>");
}

TEST_P(PHPGrammarFixture, ValidatesComparisonAndTernaryChains) {
    for (const char *source : {
             "<?php $x = 1 < 2 < 3; ?>", "<?php $x = $a == $b == $c; ?>",
             "<?php $x = $a <=> $b === $c; ?>"})
        rejects(source, "PHP2003");
    for (const char *source : {
             "<?php $x = $a ? $b : $c ? $d : $e; ?>",
             "<?php $x = $a ?: $b ? $c : $d; ?>",
             "<?php $x = $a ? $b : $c ?: $d; ?>"})
        rejects(source, "PHP2004");
    for (const char *source : {
             "<?php $x = (1 < 2) < 3; $y = 1 < (2 < 3); ?>",
             "<?php $x = 1 == (2 == 3); $y = 1 == 2 < 3; ?>",
             "<?php $x = $a ?: $b ?: $c; ?>",
             "<?php $x = $a ? ($b ? $c : $d) : $e; ?>",
             "<?php $x = $a ? $b ? $c : $d : $e; ?>",
             "<?php $x = ($a ? $b : $c) ? $d : $e; ?>",
             "<?php $x = $a ? $b : ($c ? $d : $e); ?>"})
        accepts(source);
}

TEST_P(PHPGrammarFixture, ValidatesStructuredInterpolation) {
    for (const char *source : {
             R"(<?php echo "{$}"; ?>)", R"(<?php echo "{$a+1}"; ?>)",
             R"(<?php echo "{$a[}"; ?>)", R"(<?php echo "${}"; ?>)",
             R"(<?php echo "$a[]"; ?>)", R"(<?php echo "$a[-foo]"; ?>)",
             R"(<?php echo "{$a"; ?>)", R"(<?php echo `{$}`; ?>)"})
        rejects(source, "PHP2005");
    for (const char *source : {
             R"(<?php echo "{$a['key']}", "{$a["key"]}", "{$foo()}"; ?>)",
             R"(<?php echo "${name}", "${1+2}", "{$a?->name}"; ?>)",
             R"(<?php echo "\{$}", "\${}", '\{$}'; ?>)",
             R"(<?php echo "$a[key] $a[$key] $a[0x1] $a[1_000] $a[-01]"; ?>)",
             R"(<?php echo "{$a["{$b}"]}"; ?>)",
             R"(<?php echo "{$a->{"name"}}"; ?>)"})
        accepts(source);
    rejects("<?php $s = <<<END\n{$}\nEND; ?>", "PHP2005");
    accepts("<?php $s = <<<'END'\n{$}\nEND; ?>");
}

TEST_P(PHPGrammarFixture, ValidatesHeredocAndNowdocIndentation) {
    for (const char *source : {
             "<?php $s = <<<END\n   a\n    END; ?>",
             "<?php $s = <<<'END'\n   a\n    END; ?>",
             "<?php $s = <<<END\n\ta\n    END; ?>",
             "<?php $s = <<<END\n    a\n \tEND; ?>",
             "<?php $s = <<<END\n\t \n    END; ?>"})
        rejects(source, "PHP2006");
    for (const char *source : {
             "<?php $s = <<<END\n    a\n    END; ?>",
             "<?php $s = <<<END\n\ta\n\tEND; ?>",
             "<?php $s = <<<END\n\n  \n    a\n    END; ?>",
             "<?php $s = <<<END\nEND; ?>",
             "<?php $s = <<<\"END\"\r\n    a\r\n    END; ?>",
             "<?php $s = <<<'END'\n  a\n  END; ?>"})
        accepts(source);
}

TEST_P(PHPGrammarFixture, UnpackedCallArguments) {
    for (const char *source : {
             "<?php strlen(...$args); ?>",
             "<?php f(...$args,); ?>",
             "<?php f(1, ...$args); ?>",
             "<?php f(...$first, ...$second); ?>",
             "<?php f(...$args, name: 1); ?>",
             "<?php f(... /* between */ $args /* trailing */,); ?>",
             "<?php f(...\n$args); ?>",
             "<?php f(...g(...$args)); ?>",
             "<?php f(...($args ?: [])); ?>",
             "<?php $obj->method(...$args); Foo::method(...$args); $fn(...$args); ?>"}) {
        accepts(source);
        EXPECT_NE(find(result.node, "CallSuffix"), nullptr);
        EXPECT_NE(find(result.node, "ArgumentList"), nullptr);
        EXPECT_NE(find(result.node, "Argument"), nullptr);
        EXPECT_NE(find(result.node, "SpreadOperator"), nullptr);
    }
}

TEST_P(PHPGrammarFixture, CallablePlaceholderRequiresClosingParenthesis) {
    for (const char *source : {
             "<?php $f = strlen(...); ?>",
             "<?php $f = strlen(... /* trivia */ ); ?>",
             "<?php $f = Foo::method(...); ?>",
             "<?php $f = $obj->method(...); ?>",
             "<?php $f = $fn(...\n); ?>"}) {
        accepts(source);
        EXPECT_NE(find(result.node, "CallSuffix"), nullptr);
        EXPECT_NE(find(result.node, "SpreadOperator"), nullptr);
        EXPECT_EQ(find(result.node, "ArgumentList"), nullptr);
        EXPECT_EQ(find(result.node, "Argument"), nullptr);
    }
}

TEST_P(PHPGrammarFixture, ValidatesBuiltinArgumentCounts) {
    for (const char *source : {
             "<?php strlen('a'); ?>",
             "<?php STRLEN('a'); ?>",
             "<?php strlen('a',); ?>",
             "<?php strlen(string: 'a'); ?>",
             "<?php strlen(...$args); ?>",
             "<?php array_merge(); ?>",
             "<?php array_merge([1], [2], [3]); ?>",
             "<?php array_merge(...$args); ?>",
             "<?php array_diff([1]); ?>",
             "<?php sprintf('literal'); ?>",
             "<?php sizeof([1]); ?>",
             "<?php ldap_connect('uri', 389); ?>",
             "<?php ldap_connect('uri', 389, 'wallet', 'password', 0); ?>",
             "<?php cli_get_process_title(); ?>",
             "<?php \\strlen('a'); ?>",
             "<?php strlen(substr('a', 0)); ?>"})
        accepts(source);

    for (const char *source : {
             "<?php strlen(); ?>",
             "<?php strlen(/* trivia */); ?>",
             "<?php strlen('a', 'b'); ?>",
             "<?php strlen(...$args, 'b'); ?>",
             "<?php array_diff(); ?>",
             "<?php sprintf(); ?>",
             "<?php sizeof(); ?>",
             "<?php ldap_connect('uri', 389, 'wallet', 'password', 0, 1); ?>",
             "<?php cli_get_process_title(1); ?>",
             "<?php \\strlen(); ?>"})
        rejects(source, "PHP2008");
}

TEST_P(PHPGrammarFixture, ArityChecksSkipNonBuiltinCallees) {
    for (const char *source : {
             "<?php function strlen($a, $b) {} strlen(1, 2); ?>",
             "<?php $obj->strlen(); ?>",
             "<?php Foo::strlen(); ?>",
             "<?php $fn(); ?>",
             "<?php Foo\\strlen(); ?>",
             "<?php strlen(...); ?>",
             "<?php new Foo(1, 2); ?>"})
        accepts(source);
}

TEST_P(PHPGrammarFixture, ArityDiagnosticIsExposedThroughValidationAPI) {
    rejects("<?php strlen(); ?>", "PHP2008");
    auto *validation = textparser_validate_php(parser.get());
    ASSERT_NE(validation, nullptr);
    ASSERT_EQ(validation->len, 1);
    EXPECT_STREQ(validation->items[0]->text,
                 "PHP2008: Function [strlen] requires at least 1 arguments, but 0 were provided");
    EXPECT_EQ(validation->items[0]->position, 6u);
    EXPECT_EQ(validation->items[0]->length, 6u);
    textparser_validation_clear(validation);
}

TEST_P(PHPGrammarFixture, RejectsMalformedUnpackingAndPlaceholders) {
    for (const char *source : {
             "<?php f(...,); ?>",
             "<?php f(..., $args); ?>",
             "<?php f(1, ...); ?>",
             "<?php f(...$args, ...); ?>",
             "<?php f(...$args ?>",
             "<?php f(...$args +); ?>"}) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        EXPECT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    }
}

TEST_P(PHPGrammarFixture, ValidationAPIExposesDiagnosticsAndResets) {
    EXPECT_EQ(textparser_php_register_validators(nullptr), -1);
    EXPECT_EQ(textparser_validate_php(nullptr), nullptr);
    const char *source = "<?php 1 = 2; ?>";
    rejects(source, "PHP2001");
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_EQ(diagnostic.start_pos, 6u);
    EXPECT_EQ(diagnostic.length, 1u);
    for (int i = 0; i < 2; ++i) {
        auto *validation = textparser_validate_php(parser.get());
        ASSERT_NE(validation, nullptr);
        ASSERT_EQ(validation->len, 1);
        EXPECT_STREQ(validation->items[0]->text, "PHP2001: Invalid assignment target.");
        EXPECT_EQ(validation->items[0]->position, 6u);
        textparser_validation_clear(validation);
    }
    accepts("<?php $a = 2; ?>");
    EXPECT_EQ(textparser_validate_php(parser.get()), nullptr);
}

TEST_P(PHPGrammarFixture, StringValidationHandlesWideEncodingsAndEmbeddedNul) {
    const std::u16string source = u"<?php echo \"\u00e9 {$}\"; ?>";
    ASSERT_EQ(parser.openmem(reinterpret_cast<const char *>(source.data()),
                             source.size() * sizeof(char16_t), TEXTPARSER_ENCODING_UTF_16), 0);
    ASSERT_EQ(textparser_php_register_validators(parser.get()), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    textparser_diagnostic diagnostic{};
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "PHP2005");
    EXPECT_EQ(diagnostic.start_pos, 11u);
    EXPECT_EQ(diagnostic.length, 7u);

    parser.reset();
    const char nul_source[] = "<?php echo \"a\0{$}\"; ?>";
    ASSERT_EQ(parser.openmem(nul_source, sizeof(nul_source) - 1, TEXTPARSER_ENCODING_UTF_8), 0);
    ASSERT_EQ(textparser_php_register_validators(parser.get()), 0);
    ASSERT_EQ(parser.parse(definition), 0);
    ASSERT_EQ(parser.execute_language_grammar(definition, &result), 0);
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 1u);
    ASSERT_EQ(textparser_get_diagnostic(parser.get(), 0, &diagnostic), 0);
    EXPECT_STREQ(diagnostic.code, "PHP2005");
}

TEST_P(PHPGrammarFixture, PHP85ExtendedConformanceCorpus) {
    for (const char *body : {
        "namespace Foo\\Bar;\nuse A\\B as C;\nuse function f;\nuse const G;\n$c = new C();",
        "namespace Foo {\n    function f() {}\n    $x = 1;\n}",
        "interface I { public function f(): int; const X = 1; }",
        "trait T { public function f() {} }",
        "abstract class A { abstract protected function f(); }",
        "final class F {}",
        "readonly class R { public function __construct(public int $x) {} }",
        "enum Suit { case Hearts; case Spades; }",
        "enum Status: string { case Active = 'a'; public function label(): string { return $this->value; } }",
        "enum E implements I { case A; const C = 1; }",
        "class C extends B implements I, J { use T; }",
        "class C { public int $a = 1; protected static ?string $b = null; private array $c = []; }",
        "class C { public const int X = 1; final protected const Y = 2; }",
        "class C { public function __construct(private readonly int $x, protected string $y = 'a') {} }",
        "class C { public function f(): static { return $this; } public function g(): never { throw new E(); } }",
        "class C { public function f(?int $x, int|string $y, A&B $z): int|false { return 1; } }",
        "$f = function ($x) use (&$y) { return $x; };",
        "$f = static function () {};",
        "$f = fn($x) => $x + 1;",
        "$f = static fn() => 1;",
        "$x = match($a) { 1, 2 => 'a', default => 'b', };",
        "$x = match(true) { $a > 1 => 1, default => 0, };",
        "foreach ($a as $k => &$v) {}",
        "foreach ($a as [$x, $y]) {}",
        "do { $x++; } while ($x < 10);",
        "for ($i = 0, $j = 1; $i < 10; $i++, $j--) {}",
        "$x = 1; $x += 1; $x -= 1; $x *= 2; $x /= 2; $x %= 2; $x **= 2; $x .= 'a'; $x &= 1; $x |= 1; $x ^= 1; $x <<= 1; $x >>= 1; $x ?" "?= 5;",
        "$x = $a and $b or $c xor $d;",
        "$x = !$a; $y = ~$a;",
        "$x = $a instanceof B;",
        "$x = (int) $a; $y = (float) $a; $z = (string) $a; $w = (bool) $a; $v = (array) $a; $u = (object) $a;",
        "$x = $a <=> $b;",
        "$x = $obj?->a?->b?->c;",
        "$x = [1, 2, 3,]; $y = ['a' => 1, ...$z];",
        "$x = [&$a, &$b];",
        "[$a, [$b, $c]] = $d; ['k' => $v] = $d; list('k' => $v) = $d;",
        "echo 1, 2, 3;",
        "print 'a';",
        "$x = include 'f.php'; $y = require 'f.php'; $z = include_once 'f.php'; $w = require_once 'f.php';",
        "exit; exit(1); die('msg');",
        "function gen() { $x = yield; $y = yield 1; $z = yield $k => $v; }",
        "function gen() { yield from [1,2]; }",
        "$x = fn() => throw new E();",
        "$x = $a ? throw new E() : 1;",
        "goto end; end: echo 1;",
        "$x = $a[1]; $y = $a['k']; $a[] = 1;",
        "Foo::$bar; Foo::CONST; Foo::class; $obj::$bar;",
        "$x = new Foo; $y = new Foo(); $z = new $class(); $w = new (expr)();",
        "$x = \\strlen('a');",
        "$x = __DIR__ . __FILE__ . __LINE__ . __FUNCTION__ . __CLASS__ . __METHOD__ . __NAMESPACE__ . __TRAIT__;",
        "$x = PHP_EOL;",
        "#[Attribute(Attribute::TARGET_CLASS)] class A {}",
        "#[A(1, name: 'x')] function f() {}",
        "class B { public function f() {} } class C extends B { #[\\Override] public function f() {} }",
        "$x = <<<EOT\n  text {$a} more\n  EOT;",
        "$x = <<<'EOT'\nraw $a\nEOT;",
        "$x = <<<EOT\nEOT;",
        "declare(strict_types=1);",
        "declare(ticks=1) { echo 1; }",
        "static $x = 1; global $y;",
        "unset($a, $b); isset($a, $b); empty($a);",
        "$x = clone $obj;",
        "$x = $a ?: $b; $y = $a ? $b : $c;",
        "$x = (function () { return 1; })();",
        "$x = $a[0] ?? $b['k'] ?? null;",
        "$x = \"a $a b {$a->b} c {$a['k']} d\";",
        "function f(int ...$args) {} f(1,2,3);",
        "function f(&$x) {} function g(&...$xs) {}",
        "class C { public function __invoke() {} public function __get($n) {} }",
        "class C { public string $x { get => $this->x; set => $this->x = $value; } }",
        "class C { public private(set) int $x = 0; }",
        "class C { public function __construct(public final int $x) {} }",
        "const X = 1; define('Y', 2);",
        "class C { public const string X = 'a'; }",
        "$x = [1,2][0];",
        "$x = (new Foo())->bar();",
        "$x = new class {};",
        "$x = new class(1) extends B implements I {};",
        "interface I { public function f(): static; }",
        "trait T { abstract public function f(); }",
        "$x = $obj->{'method'}(); $y = $obj->$name();",
        "$x = $a::CONST; $y = $a::method();",
        "$x = static::foo(); $y = parent::foo(); self::foo();",
        "function f(): void {} function g(): ?int { return null; }",
        "$x = fn(int $a): int => $a;",
        "class C { public function f(): (A&B)|C { return $x; } }",
        "function f(A&B $x) {}",
        "$x = $a?->b?->c();",
        "$x = $a ?? throw new E();",
        "echo <<<EOT\nhi\nEOT;",
        "$x = +$a - -$b;",
        "$x = $a ** $b ** $c;",
        "$x = ($a + $b) * $c;",
        "$x = 0b1010 + 0o17 + 0xFF + 1_000 + 1.5e3;",
        "$x = 'single \\' quote'; $y = \"double \\\" quote \\\\ \\$a\";",
        "$x = $a |> strlen(...) |> strtoupper(...);",
        "class B { public function __construct() {} } class C extends B { public function __construct() { parent::__construct(); } }",
        "class C { public static function f() { return new static(); } }",
        "$x = $a === $b && $c !== $d || $e == $f;",
        "$x = $a << 2 | $b >> 1 & $c ^ $d;",
        "$x = match($a) { default => 1 };",
        "foreach ($a as $v) : echo $v; endforeach;",
        "if ($a) : elseif ($b) : else : endif;",
        "while ($a) : endwhile; for (;;) : endfor;",
        "$x = function () use ($a, &$b) : int { return 1; };",
        "class C { public function f() { return $this->g(...); } }",
        "$x = $a[0][1]->b['c'];",
        "$x = -$a ** 2;",
        "$x = 1 + 2 * 3 - 4 / 5 % 6;",
        "$x = 'a' . 'b' . 'c';",
        "$x = $a ?? $b ?: $c;",
        "class C { var $x = 1; }",
        "abstract class C { abstract public function f(); }",
        "function f($a = 1, $b = [1, 2], $c = null) {}",
        "function f(): int|string|null { return null; }",
        "function f(?A $x = null): ?B { return null; }",
        "class C { public function f(): array { return []; } }",
        "$x = array(1, 2, 3);",
        "$x = array('a' => 1, 'b' => 2);",
        "$x = list($a, $b) = [1, 2];",
        "$x = isset($a) ? $a : $b;",
        "$x = $a instanceof static;",
        "$x = $a instanceof $b;",
    }) {
        SCOPED_TRACE(body);
        const std::string source = std::string("<?php ") + body + " ?>";
        accepts(source.c_str());
    }
}

TEST_P(PHPGrammarFixture, PHP85ExtendedRecoveryChecks) {
    for (const char *source : {
        "<?php $x = ;",
        "<?php function f( {}",
        "<?php class {}",
        "<?php if ($x { }",
        "<?php $x = [1,2;",
        "<?php echo ;",
        "<?php $x = 1 +;",
        "<?php foreach ($a as) {}",
        "<?php switch ($x) { case: }",
        "<?php namespace;",
        "<?php use;",
        "<?php $x = match($a) { => 1 };",
        "<?php function f() { $x = ; }",
        "<?php class C { public $x = ; }",
        "<?php $a = ; $b = ; echo ;",
        "<?php while ($a) {",
        "<?php do { $x++; }",
        "<?php try { } catch",
        "<?php $x = (1 + 2;",
        "<?php $x = [1, 2,]; $y = ;",
        "<?php fn($x) => ;",
        "<?php $x = match($a) { 1 => };",
        "<?php interface I { public function f() }",
        "<?php enum E { case }",
        "<?php $x = new;",
        "<?php return; $y = ;",
    }) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        EXPECT_TRUE(result.status != TEXTPARSER_MATCH_OK ||
                    textparser_get_diagnostic_count(parser.get()) > 0);
    }
}

TEST_P(PHPGrammarFixture, PHP85RecoveryContinuesAtMemberBoundaries) {
    ASSERT_NO_FATAL_FAILURE(parse("<?php class C { public $x = ; public $y = 1; } echo 1; ?>"));
    ASSERT_EQ(textparser_get_diagnostic_count(parser.get()), 2u);
    for (size_t i = 0; i < 2; ++i) {
        textparser_diagnostic diagnostic{};
        ASSERT_EQ(textparser_get_diagnostic(parser.get(), i, &diagnostic), 0);
        EXPECT_STREQ(diagnostic.code, "PHP1002");
    }
}

TEST_P(PHPGrammarFixture, PHP85UnterminatedConstructsDoNotHang) {
    for (const char *source : {
             "<?php if (true) {",
             "<?php class C {",
             "<?php function f() {",
             "<?php $x = [1, 2",
             "<?php $x = (1 + 2",
             "<?php echo \"unterminated",
             "<?php $x = 1 +",
             "<?php $x = <<<END\nunterminated",
             "<?php /* unterminated"}) {
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(parse(source));
        EXPECT_TRUE(result.status != TEXTPARSER_MATCH_OK ||
                    textparser_get_diagnostic_count(parser.get()) > 0);
    }
}

TEST_P(PHPGrammarFixture, PHP85GrammarGapRegressions) {
    for (const char *source : {
             "<?php $x |= 1; ?>",
             "<?php $x ^= 1; ?>",
             "<?php $x |= 1; $x ^= 2; $x &= 3; ?>",
             "<?php $x = new (expr); ?>",
             "<?php $x = new (expr)(); ?>",
             "<?php $x = new ($a . 'b')(); ?>",
             "<?php namespace { } ?>",
             "<?php namespace Foo { } ?>"})
        accepts(source);
    rejects("<?php namespace; ?>", "PHP1002");
}

TEST_P(PHPGrammarFixture, PHP85RejectsEmptyArrayElements) {
    for (const char *source : {
             "<?php $x = [,,]; ?>",
             "<?php $x = [,]; ?>",
             "<?php $x = [1,,2]; ?>",
             "<?php $x = [[, $y]]; ?>",
             "<?php $x = array(,); ?>",
             "<?php $x = array(1,,2); ?>",
             "<?php f([, $x]); ?>",
             "<?php foreach ([, $x] as $v) {} ?>",
             "<?php list($a, [, $b]) = $x; ?>",
             "<?php [, $x] = [, $y]; ?>"})
        rejects(source, "PHP2009");

    for (const char *source : {
             "<?php $x = []; ?>",
             "<?php $x = [1,]; ?>",
             "<?php $x = [1, 2,]; ?>",
             "<?php $x = array(1,); ?>",
             "<?php [, $x] = $a; ?>",
             "<?php [$a, [, $b]] = $x; ?>",
             "<?php foreach ($a as [, $x]) {} ?>",
             "<?php foreach ($a as $k => [, $x]) {} ?>",
             "<?php list(, $x) = $a; ?>",
             "<?php list(, $x, ) = $a; ?>"})
        accepts(source);

    rejects("<?php $x = [,,]; ?>", "PHP2009");
    auto *validation = textparser_validate_php(parser.get());
    ASSERT_NE(validation, nullptr);
    ASSERT_EQ(validation->len, 1);
    EXPECT_STREQ(validation->items[0]->text,
                 "PHP2009: Cannot use empty array elements in arrays.");
    textparser_validation_clear(validation);
}

INSTANTIATE_TEST_SUITE_P(DefinitionPaths, PHPGrammarFixture, testing::Values(false, true),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "Static" : "JSON"; });

} // namespace
