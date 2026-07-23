#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <textparser.h>
#pragma GCC diagnostic pop


#include <cstring>

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
            children = textparser_get_token_children_count(token);
            if (handle)
            {
                char *txt = textparser_get_token_text(handle, token);
                if (txt)
                {
                    value = txt;
                    free(txt);
                }
            }
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

        return TokenParserItem(token, m_definition, m_handle);
    }

    int position = -1;
    int length = 0;
    const char *type = nullptr;
    size_t children = 0;
    std::string value;

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
                count++;
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

        for(size_t c = 0; c < index; c++)
        {
            if (token == nullptr) break;
            token = token->next;
        }

        return TokenParserItem(token, m_definition, m_handle);
    }

    size_t count = 0;

private:
    textparser_t m_handle = nullptr;
    const textparser_language_definition *m_definition = nullptr;
};
