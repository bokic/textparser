#pragma once

#include <textparser.hpp>

// Preserve the zero defaults of partially initialized test productions.
inline textparser_production make_test_production(
    int id, const char *name, textparser_production_kind kind,
    const int *children, size_t child_count, int token_id,
    int referenced_production, const char *predicate_name = nullptr,
    const char *context_name = nullptr, int64_t context_value = 0) {
    textparser_production production{};
    production.id = id;
    production.name = name;
    production.kind = kind;
    production.children = children;
    production.child_count = child_count;
    production.token_id = token_id;
    production.referenced_production = referenced_production;
    production.predicate_name = predicate_name;
    production.context_name = context_name;
    production.context_value = context_value;
    return production;
}
