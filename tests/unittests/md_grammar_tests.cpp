#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <md_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct MDGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/md_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &md_definition;
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
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] "
                              << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources,
                         MDGrammarFixture,
                         testing::Values(false, true),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

TEST_P(MDGrammarFixture, parses_basic_markdown_document) {
    const char *source = R"MD_SRC(# Document Title

<!-- This is a comment -->

Here is a paragraph with **bold text**, *italic text*, and `inline code`.
Strikethrough is also supported: ~~deleted words~~.

## Section 1: Code and Blockquotes

```python
def hello_world():
    print("Hello from code block!")
```

> This is a blockquote.
> It spans multiple lines.

---

### Section 2: Lists and Links

- Unordered item 1
- Unordered item 2
  * Sub-item A
  + Sub-item B

1. First ordered step
2. Second ordered step

- [ ] Task incomplete
- [x] Task completed

Check out [Google](https://google.com) and an image: ![Logo](https://example.com/logo.png).
Here is a footnote reference[^1] and escaped asterisk: \*not italic\*.

| Header 1 | Header 2 |
|----------|----------|
| Value 1  | Value 2  |

<div class="note">HTML content</div>
)MD_SRC";

    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->category, TEXTPARSER_CST_SOURCE_FILE);

    EXPECT_NE(find(root, "Heading"), nullptr);
    EXPECT_NE(find(root, "HtmlComment"), nullptr);
    EXPECT_NE(find(root, "Paragraph"), nullptr);
    EXPECT_NE(find(root, "Bold"), nullptr);
    EXPECT_NE(find(root, "Italic"), nullptr);
    EXPECT_NE(find(root, "InlineCode"), nullptr);
    EXPECT_NE(find(root, "Strikethrough"), nullptr);
    EXPECT_NE(find(root, "FencedCodeBlock"), nullptr);
    EXPECT_NE(find(root, "Blockquote"), nullptr);
    EXPECT_NE(find(root, "HorizontalRule"), nullptr);
    EXPECT_NE(find(root, "UnorderedList"), nullptr);
    EXPECT_NE(find(root, "OrderedList"), nullptr);
    EXPECT_NE(find(root, "TaskCheckbox"), nullptr);
    EXPECT_NE(find(root, "Link"), nullptr);
    EXPECT_NE(find(root, "Image"), nullptr);
    EXPECT_NE(find(root, "Footnote"), nullptr);
    EXPECT_NE(find(root, "BackslashEscape"), nullptr);
    EXPECT_NE(find(root, "Table"), nullptr);
    EXPECT_NE(find(root, "HtmlTag"), nullptr);
}

TEST_P(MDGrammarFixture, parses_atx_headings_all_levels) {
    const char *source = R"(
# Level 1 Heading
## Level 2 Heading
### Level 3 Heading
#### Level 4 Heading
##### Level 5 Heading
###### Level 6 Heading
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Heading"), nullptr);
}

TEST_P(MDGrammarFixture, parses_fenced_code_blocks) {
    const char *source = R"(
```
plain block
```

~~~
tilde block
~~~

```c
int main() { return 0; }
```

```
```
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "FencedCodeBlock"), nullptr);
}

TEST_P(MDGrammarFixture, parses_thematic_breaks_horizontal_rules) {
    const char *source = R"(
---
***
___
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "HorizontalRule"), nullptr);
}

TEST_P(MDGrammarFixture, parses_blockquotes) {
    const char *source = R"(
> First line of blockquote
> Second line of blockquote
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Blockquote"), nullptr);
}

TEST_P(MDGrammarFixture, parses_lists_unordered_and_ordered) {
    const char *source = R"(
- Dash list item
* Asterisk list item
+ Plus list item

1. Numbered item one
2. Numbered item two
3) Paren numbered item
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "UnorderedList"), nullptr);
    EXPECT_NE(find(root, "OrderedList"), nullptr);
}

TEST_P(MDGrammarFixture, parses_task_checkbox_lists) {
    const char *source = R"(
- [ ] Task to do
- [x] Task completed lowercase
- [X] Task completed uppercase
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "TaskCheckbox"), nullptr);
}

TEST_P(MDGrammarFixture, parses_pipe_tables) {
    const char *source = R"(
| Col A | Col B | Col C |
|:------|:-----:|------:|
| 1     | 2     | 3     |
| 4     | 5     | 6     |
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Table"), nullptr);
    EXPECT_NE(find(root, "TableRow"), nullptr);
}

TEST_P(MDGrammarFixture, parses_inline_formatting_varieties) {
    const char *source = R"(
Paragraph with **bold asterisks** and __bold underscores__.
Also *italic asterisks* and _italic underscores_.
Plus ***bold italic*** and ~~strikethrough text~~.
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Bold"), nullptr);
    EXPECT_NE(find(root, "Italic"), nullptr);
    EXPECT_NE(find(root, "BoldItalic"), nullptr);
    EXPECT_NE(find(root, "Strikethrough"), nullptr);
}

TEST_P(MDGrammarFixture, parses_nested_inline_formatting) {
    const char *source = R"(
**bold with *nested italic* inside**
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Bold"), nullptr);
    EXPECT_NE(find(root, "Italic"), nullptr);
}

TEST_P(MDGrammarFixture, parses_inline_code_spans) {
    const char *source = R"(
Here is `inline_code()` and ``code with ` backtick inside``.
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "InlineCode"), nullptr);
}

TEST_P(MDGrammarFixture, parses_links_and_images) {
    const char *source = R"(
[OpenAI](https://openai.com)
[Reference link][1]
![Alt text](image.png "Title")
![Image ref][img-ref]
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Link"), nullptr);
    EXPECT_NE(find(root, "Image"), nullptr);
}

TEST_P(MDGrammarFixture, parses_footnotes) {
    const char *source = R"(
Here is a footnote[^1] and a named one[^note-xyz].
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Footnote"), nullptr);
}

TEST_P(MDGrammarFixture, parses_backslash_escapes) {
    const char *source = R"(
\*not bold\*
\# not a heading
\[not a link\]
\`not code\`
\\not escape
\~not strikethrough\~
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "BackslashEscape"), nullptr);
}

TEST_P(MDGrammarFixture, parses_html_elements_and_comments) {
    const char *source = R"(
<div class="card" id="main">
    <span style='color:red'>Text</span>
    <img src="avatar.jpg" alt="profile" />
    <br/>
    <!-- HTML Comment -->
</div>
)";
    textparser_node *root = parse_source(source);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "HtmlTag"), nullptr);
    EXPECT_NE(find(root, "HtmlComment"), nullptr);
}

TEST_P(MDGrammarFixture, parses_empty_and_plain_text) {
    textparser_node *root_empty = parse_source("");
    (void)root_empty;

    textparser_node *root_plain = parse_source("Just plain prose without markdown formatting.");
    ASSERT_NE(root_plain, nullptr);
    EXPECT_NE(find(root_plain, "Paragraph"), nullptr);
}

TEST_P(MDGrammarFixture, error_recovery_after_invalid_construct) {
    const char *source = R"(
# Valid Heading 1

Some paragraph with [unclosed link

# Next Valid Heading 2
)";
    textparser_node *root = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(root, nullptr);
    EXPECT_NE(find(root, "Heading"), nullptr);
}

} // namespace
