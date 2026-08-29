#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <textparser.hpp>
#pragma GCC diagnostic pop


#include <cstring>
#include <functional>
#include <print>
#include <string>


inline bool &textparser_suppress_errors() {
    static bool suppress = false;
    return suppress;
}


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
        , type(other.type)
        , children(other.children)
        , value(other.value)
        , m_token(other.m_token)
        , m_definition(other.m_definition)
        , m_handle(other.m_handle)
    {}
    TokenParserItem& operator=(const TokenParserItem &other)
    {
        position = other.position;
        length = other.length;
        type = other.type;
        children = other.children;
        value = other.value;
        m_token = other.m_token;
        m_definition = other.m_definition;
        m_handle = other.m_handle;

        return *this;
    }
    TokenParserItem(const TokenParserItem &&other) = delete;
    TokenParserItem& operator=(const TokenParserItem &&other) = delete;

    TokenParserItem(const textparser_token_item *token, const textparser_language_definition *definition, textparser_t handle = nullptr)
        : m_token(token)
        , m_definition(definition)
        , m_handle(handle)
    {
        if (token)
        {
            position = textparser_get_token_position(token);
            length = textparser_get_token_length(token);
            type = textparser_get_token_type_str(definition, token);
            children = 0;
            const textparser_token_item *c = token->child;
            while (c) {
                if (c->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && c->token_id != TEXTPARSER_TOKEN_ID_WHITESPACE) {
                    children++;
                }
                c = c->next;
            }
            if (handle)
            {
                char *txt = textparser_get_token_text(handle, token);
                if (txt)
                {
                    value = txt;
                    textparser_free_token_text(txt);
                }
            }
        }
    }

    TokenParserItem operator[](size_t index) const
    {
        const textparser_token_item *token = m_token ? m_token->child : nullptr;

        size_t c = 0;
        while (token != nullptr)
        {
            if (token->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && token->token_id != TEXTPARSER_TOKEN_ID_WHITESPACE)
            {
                if (c == index) break;
                c++;
            }
            token = token->next;
        }

        return TokenParserItem(token, m_definition, m_handle);
    }

    int position = -1;
    int length = 0;
    const char *type = nullptr;
    size_t children = 0;
    std::string value;

    const textparser_token_item *raw_token() const { return m_token; }

private:
    const textparser_token_item *m_token = nullptr;
    const textparser_language_definition *m_definition = nullptr;
    textparser_t m_handle = nullptr;
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
                if (token->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && token->token_id != TEXTPARSER_TOKEN_ID_WHITESPACE) {
                    count++;
                }
                token = token->next;
            }
        }
        else if (!textparser_suppress_errors())
        {
            std::println(stderr, "Parsing error: {}, at position: {}", textparser_parse_error(m_handle), textparser_parse_error_position(m_handle));
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

        size_t c = 0;
        while (token != nullptr)
        {
            if (token->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && token->token_id != TEXTPARSER_TOKEN_ID_WHITESPACE)
            {
                if (c == index) break;
                c++;
            }
            token = token->next;
        }

        return TokenParserItem(token, m_definition, m_handle);
    }

    TokenParserItem at(size_t index) const
    {
        return operator[](index);
    }

    void post_process()
    {
        if (m_handle && m_definition)
        {
            textparser_token_item *root = textparser_get_first_token(m_handle);
            textparser_post_process(&root, m_definition);
            count = 0;
            auto token = textparser_get_first_token(m_handle);
            while (token)
            {
                if (token->token_id != TEXTPARSER_TOKEN_ID_UNPROCESSED && token->token_id != TEXTPARSER_TOKEN_ID_WHITESPACE) {
                    count++;
                }
                token = token->next;
            }
        }
    }

    size_t count = 0;

private:
    textparser_t m_handle = nullptr;
    const textparser_language_definition *m_definition = nullptr;
};

inline bool has_token_type(const TextParser &tokens, const char *type_name) {
    std::function<bool(const TokenParserItem&)> scan = [&](const TokenParserItem &item) {
        if (item.type && strcmp(item.type, type_name) == 0) {
            return true;
        }
        for (size_t i = 0; i < item.children; ++i) {
            if (scan(item[i])) return true;
        }
        return false;
    };
    for (size_t i = 0; i < tokens.count; ++i) {
        if (scan(tokens[i])) return true;
    }
    return false;
}
