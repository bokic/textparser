#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>
#include <vector>

#include <md_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_MD, basic_markdown_document) {
    auto tokens = TextParser(R"(# Document Title

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

)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Heading"));
    EXPECT_TRUE(found.contains("HtmlComment"));
    EXPECT_TRUE(found.contains("Bold"));
    EXPECT_TRUE(found.contains("Italic"));
    EXPECT_TRUE(found.contains("InlineCode"));
    EXPECT_TRUE(found.contains("Strikethrough"));
    EXPECT_TRUE(found.contains("FencedCodeBlock"));
    EXPECT_TRUE(found.contains("Blockquote"));
    EXPECT_TRUE(found.contains("HorizontalRule"));
    EXPECT_TRUE(found.contains("UnorderedList"));
    EXPECT_TRUE(found.contains("OrderedList"));
    EXPECT_TRUE(found.contains("TaskCheckbox"));
    EXPECT_TRUE(found.contains("Link"));
    EXPECT_TRUE(found.contains("Image"));
    EXPECT_TRUE(found.contains("Footnote"));
    EXPECT_TRUE(found.contains("BackslashEscape"));
    EXPECT_TRUE(found.contains("TablePipe"));
    EXPECT_TRUE(found.contains("HtmlTag"));
}

TEST(parse_MD, headings_levels) {
    auto tokens = TextParser(R"(
# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Heading"));
}

TEST(parse_MD, fenced_code_blocks) {
    auto tokens = TextParser(R"(
```
plain block
```

~~~
tilde block
~~~

```c
int main() { return 0; }
```
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("FencedCodeBlock"));
}

TEST(parse_MD, horizontal_rules) {
    auto tokens = TextParser(R"(
---
***
___
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("HorizontalRule"));
}

TEST(parse_MD, bold_and_italic_flavors) {
    auto tokens = TextParser(R"(
**bold with asterisks**
__bold with underscores__
*italic with asterisk*
_italic with underscore_
**bold with *nested italic* inside**
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("Bold"));
    EXPECT_TRUE(found.contains("Italic"));
}

TEST(parse_MD, html_tags_and_attributes) {
    auto tokens = TextParser(R"(
<span id="test" class='example'>Text</span>
<img src="img.jpg" alt="alt text" />
<br/>
<!-- Comment here -->
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("HtmlTag"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("Equal"));
    EXPECT_TRUE(found.contains("AttributeName"));
    EXPECT_TRUE(found.contains("HtmlComment"));
}

TEST(parse_MD, checkboxes_and_footnotes) {
    auto tokens = TextParser(R"(
- [ ] unchecked item
- [x] checked item lowercase
- [X] checked item uppercase
Reference to footnote[^note-1] and another[^42].
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("TaskCheckbox"));
    EXPECT_TRUE(found.contains("Footnote"));
}

TEST(parse_MD, backslash_escapes) {
    auto tokens = TextParser(R"(
\*not bold\*
\# not a heading
\[not a link\]
\`not code\`
\\not escape
\~not strikethrough\~
)", &md_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("BackslashEscape"));
}

TEST(parse_MD, token_colors_and_metadata) {
    EXPECT_STREQ(md_definition.name, "md");
    EXPECT_TRUE(md_definition.case_sensitivity);
    EXPECT_STREQ(md_definition.default_file_extensions[0], "md");
    EXPECT_STREQ(md_definition.default_file_extensions[1], "markdown");

    for (int i = 0; md_definition.tokens[i].name != nullptr; i++) {
        // Verify every token has a non-null startRegexFunction
        EXPECT_NE(md_definition.tokens[i].startRegexFunction, nullptr)
            << "Token " << md_definition.tokens[i].name << " missing startRegexFunction";

        if (md_definition.tokens[i].type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            EXPECT_NE(md_definition.tokens[i].endRegexFunction, nullptr)
                << "Token " << md_definition.tokens[i].name << " missing endRegexFunction";
        }
    }
}

TEST(parse_MD, empty_and_plain_text) {
    auto tokens_empty = TextParser("", &md_definition);
    EXPECT_EQ(tokens_empty.count, 0);

    auto tokens_plain = TextParser("Just plain prose without markdown formatting.", &md_definition);
    EXPECT_GE(tokens_plain.count, 0);
}

TEST(parse_MD, multi_encoding_utf16_and_utf32) {
    // Test parsing with UTF-16
    const char16_t text16[] = u"# UTF-16 Heading\n**bold text** and `code`\n";
    textparser_t handle16 = nullptr;
    int err16 = textparser_openmem((const char *)text16, sizeof(text16) - sizeof(char16_t), TEXTPARSER_ENCODING_UTF_16, &handle16);
    ASSERT_EQ(err16, 0);
    int parse_err16 = textparser_parse(handle16, &md_definition);
    EXPECT_EQ(parse_err16, 0);
    textparser_close(handle16);

    // Test parsing with UTF-32
    const char32_t text32[] = U"# UTF-32 Heading\n*italic text* and [link](https://example.com)\n";
    textparser_t handle32 = nullptr;
    int err32 = textparser_openmem((const char *)text32, sizeof(text32) - sizeof(char32_t), TEXTPARSER_ENCODING_UTF_32, &handle32);
    ASSERT_EQ(err32, 0);
    int parse_err32 = textparser_parse(handle32, &md_definition);
    EXPECT_EQ(parse_err32, 0);
    textparser_close(handle32);
}

TEST(parse_MD, token_export_buffer) {
    const char *doc = "# Title\n\n```python\nprint(1)\n```\n";
    textparser_t handle = nullptr;
    int err = textparser_openmem(doc, strlen(doc), TEXTPARSER_ENCODING_LATIN1, &handle);
    ASSERT_EQ(err, 0);
    err = textparser_parse(handle, &md_definition);
    ASSERT_EQ(err, 0);

    textparser_token_range ranges[32];
    size_t written = 0;
    int exp_err = textparser_export_tokens(handle, ranges, 32, &written);
    EXPECT_EQ(exp_err, 0);
    EXPECT_GT(written, 0);

    textparser_close(handle);
}
