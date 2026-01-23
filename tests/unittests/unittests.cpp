#include <gtest/gtest.h>
#include <textparser.h>

#include <json_definition.json.h>
#include <cfml_definition.json.h>


class TokenParserItem
{
public:
    explicit TokenParserItem(const textparser_language_definition *definition)
        : m_definition(definition)
    {
    }
    ~TokenParserItem(){}
    TokenParserItem(const TokenParserItem &other)
        : position(other.position)
        , length(other.length)
        , name(other.name)
        , children(other.children)
        , m_token(other.m_token)
        , m_definition(other.m_definition)
    {}
    TokenParserItem& operator=(const TokenParserItem &other)
    {
        position = other.position;
        length = other.length;
        name = other.name;
        children = other.children;
        m_token = other.m_token;
        m_definition = other.m_definition;

        return *this;
    }
    TokenParserItem(const TokenParserItem &&other) = delete;
    TokenParserItem& operator=(const TokenParserItem &&other) = delete;

    TokenParserItem(const textparser_token_item *token, const textparser_language_definition *definition)
        : m_token(token)
        , m_definition(definition)
    {
        if (token)
        {
            position = textparser_get_token_position(token);
            length = textparser_get_token_length(token);
            name = textparser_get_token_type_str(definition, token);
            children = textparser_get_token_children_count(token);
        }
    }

    TokenParserItem operator[](size_t index) const
    {
        const textparser_token_item *token = m_token->child;

        for(size_t c = 0; c < index; c++)
        {
            if (token == nullptr) break;
            token = token->next;
        }

        return TokenParserItem(token, m_definition);
    }

    int position = -1;
    int length = 0;
    const char *name = nullptr;
    size_t children = 0;

private:
    const textparser_token_item *m_token = nullptr;
    const textparser_language_definition *m_definition = nullptr;
};

class TextParser
{
public:
    TextParser(const char *text, const textparser_language_definition *definition)
        : m_definition(definition)
    {
        textparser_openmem(text, strlen(text), definition->default_text_encoding, &m_handle);
        if (textparser_parse(m_handle, definition) == 0)
        {
            auto token = textparser_get_first_token(m_handle);
            while(token)
            {
                count++;
                token = token->next;
            }
        }
    }

    ~TextParser()
    {
        if (m_handle)
        {
            textparser_close(m_handle);
            m_handle = nullptr;
        }
    }

    TextParser(const TextParser &other) = delete;
    TextParser& operator=(const TextParser &other) = delete;
    TextParser(const TextParser &&other) = delete;
    TextParser& operator=(const TextParser &&other) = delete;

    TokenParserItem operator[](size_t index) const
    {
        const textparser_token_item *token = textparser_get_first_token(m_handle);

        for(size_t c = 0; c < index; c++)
        {
            if (token == nullptr) break;
            token = token->next;
        }

        return TokenParserItem(token, m_definition);
    }

    size_t count = 0;

private:
    textparser_t m_handle = nullptr;
    const textparser_language_definition *m_definition = nullptr;
};

TEST(parse_JSON, basic) {
    auto tokens = TextParser(R"([1,2,3])", &json_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].name, "Array");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   7);
    EXPECT_EQ   (tokens[0].children, 5);

    EXPECT_STREQ(tokens[0][0].name, "Number");
    EXPECT_EQ   (tokens[0][0].position, 1);
    EXPECT_EQ   (tokens[0][0].length,   1);
    EXPECT_EQ   (tokens[0][0].children, 0);

    EXPECT_STREQ(tokens[0][1].name, "ValueSeparator");
    EXPECT_EQ   (tokens[0][1].position, 2);
    EXPECT_EQ   (tokens[0][1].length,   1);
    EXPECT_EQ   (tokens[0][1].children, 0);

    EXPECT_STREQ(tokens[0][2].name, "Number");
    EXPECT_EQ   (tokens[0][2].position, 3);
    EXPECT_EQ   (tokens[0][2].length,   1);
    EXPECT_EQ   (tokens[0][2].children, 0);

    EXPECT_STREQ(tokens[0][3].name, "ValueSeparator");
    EXPECT_EQ   (tokens[0][3].position, 4);
    EXPECT_EQ   (tokens[0][3].length,   1);
    EXPECT_EQ   (tokens[0][3].children, 0);

    EXPECT_STREQ(tokens[0][4].name, "Number");
    EXPECT_EQ   (tokens[0][4].position, 5);
    EXPECT_EQ   (tokens[0][4].length,   1);
    EXPECT_EQ   (tokens[0][4].children, 0);
}

TEST(parse_CFML, basic_cfset) {
    auto tokens = TextParser(R"(<cfset a = 1234 />)", &cfml_definition);
    EXPECT_EQ(tokens.count, 1);

    EXPECT_STREQ(tokens[0].name, "StartTag");
    EXPECT_EQ   (tokens[0].position, 0);
    EXPECT_EQ   (tokens[0].length,   18);
    EXPECT_EQ   (tokens[0].children, 1);

    EXPECT_STREQ(tokens[0][0].name, "Expression");
    EXPECT_EQ   (tokens[0][0].position, 7);
    EXPECT_EQ   (tokens[0][0].length,   9);
    EXPECT_EQ   (tokens[0][0].children, 3);

    EXPECT_STREQ(tokens[0][0][0].name, "Variable");
    EXPECT_EQ   (tokens[0][0][0].position, 7);
    EXPECT_EQ   (tokens[0][0][0].length,   1);
    EXPECT_EQ   (tokens[0][0][0].children, 0);

    EXPECT_STREQ(tokens[0][0][1].name, "Operator");
    EXPECT_EQ   (tokens[0][0][1].position, 9);
    EXPECT_EQ   (tokens[0][0][1].length,   1);
    EXPECT_EQ   (tokens[0][0][1].children, 0);

    EXPECT_STREQ(tokens[0][0][2].name, "Number");
    EXPECT_EQ   (tokens[0][0][2].position, 11);
    EXPECT_EQ   (tokens[0][0][2].length,   4);
    EXPECT_EQ   (tokens[0][0][2].children, 0);
}
