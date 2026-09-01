#pragma once

#define TEXTPARSER_ALLOW_C_HEADER_IN_CPP

#ifdef __cplusplus
extern "C" {
#endif

#include "textparser.h"

#ifdef __cplusplus
}
#endif

#undef TEXTPARSER_ALLOW_C_HEADER_IN_CPP

#ifdef __cplusplus

#include <utility>

namespace textparser {

class State {
public:
    State() = default;

    explicit State(textparser_parser_state *state) : m_state(state) {}

    ~State() {
        reset();
    }

    State(const State &) = delete;
    State &operator=(const State &) = delete;

    State(State &&other) noexcept : m_state(other.m_state) {
        other.m_state = nullptr;
    }

    State &operator=(State &&other) noexcept {
        if (this != &other) {
            reset();
            m_state = other.m_state;
            other.m_state = nullptr;
        }
        return *this;
    }

    static State create(textparser_t handle) {
        return State(textparser_state_new(handle));
    }

    static State generate(textparser_t handle, size_t position) {
        return State(textparser_state_generate(handle, position));
    }

    textparser_parser_state *get() const { return m_state; }
    textparser_parser_state *release() {
        textparser_parser_state *state = m_state;
        m_state = nullptr;
        return state;
    }

    void reset(textparser_parser_state *state = nullptr) {
        if (m_state) {
            textparser_state_free(m_state);
        }
        m_state = state;
    }

    explicit operator bool() const { return m_state != nullptr; }

private:
    textparser_parser_state *m_state = nullptr;
};

class Parser {
public:
    Parser() = default;

    explicit Parser(textparser_t handle) : m_handle(handle) {}

    ~Parser() {
        reset();
    }

    Parser(const Parser &) = delete;
    Parser &operator=(const Parser &) = delete;

    Parser(Parser &&other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    Parser &operator=(Parser &&other) noexcept {
        if (this != &other) {
            reset();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    int openfile(const char *pathname, enum textparser_encoding default_text_format, int bom_mask) {
        reset();
        return textparser_openfile(pathname, default_text_format, bom_mask, &m_handle);
    }

    int openmem(const char *buffer, int len, enum textparser_encoding default_text_format) {
        reset();
        return textparser_openmem(buffer, len, default_text_format, &m_handle);
    }

    int set_text(const char *buffer, int len = -1) {
        return textparser_set_text(m_handle, buffer, len);
    }

    int parse(const textparser_language_definition *definition) {
        return textparser_parse(m_handle, definition);
    }

    int parse_incremental(const textparser_language_definition *definition, size_t edit_offset, size_t old_len, const void *new_text, size_t new_len, textparser_dirty_range *out_range = nullptr) {
        return textparser_parse_incremental(m_handle, definition, edit_offset, old_len, new_text, new_len, out_range);
    }

    textparser_token_item *get_first_token() const {
        return textparser_get_first_token(m_handle);
    }

    textparser_t get() const { return m_handle; }
    textparser_t release() {
        textparser_t handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

    void reset(textparser_t handle = nullptr) {
        if (m_handle) {
            textparser_close(m_handle);
        }
        m_handle = handle;
    }

    explicit operator bool() const { return m_handle != nullptr; }

    int export_tokens(textparser_token_range *buffer, size_t max_tokens, size_t *out_count) const {
        return textparser_export_tokens(m_handle, buffer, max_tokens, out_count);
    }

    int export_tokens_range(size_t start_pos, size_t end_pos, textparser_token_range *buffer, size_t max_tokens, size_t *out_count) const {
        return textparser_export_tokens_range(m_handle, start_pos, end_pos, buffer, max_tokens, out_count);
    }

    int export_tokens_lines(size_t start_line, size_t end_line, textparser_token_range *buffer, size_t max_tokens, size_t *out_count) const {
        return textparser_export_tokens_lines(m_handle, start_line, end_line, buffer, max_tokens, out_count);
    }

    const textparser_lex_token *lexer_tokens(size_t *out_count) const {
        return textparser_get_lexer_tokens(m_handle, out_count);
    }

    const textparser_lex_trivia *lexer_trivia(size_t *out_count) const {
        return textparser_get_lexer_trivia(m_handle, out_count);
    }

    int parser_state(textparser_parser_state_view *out_state) const {
        return textparser_get_parser_state(m_handle, out_state);
    }

    int execute_production(const textparser_production *productions,
                           size_t production_count,
                           int start_production,
                           textparser_match_result *out_result) const {
        return textparser_execute_production(m_handle, productions, production_count,
                                             start_production, out_result);
    }

    int execute_language_grammar(const textparser_language_definition *language,
                                 textparser_match_result *out_result) const {
        return textparser_execute_language_grammar(m_handle, language, out_result);
    }

    const char *get_parse_error() const {
        return textparser_parse_error(m_handle);
    }

    size_t get_parse_error_position() const {
        return textparser_parse_error_position(m_handle);
    }

private:
    textparser_t m_handle = nullptr;
};

} // namespace textparser

#endif
