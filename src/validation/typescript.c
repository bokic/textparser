#include "typescript.h"
#include "validation.h"

#include <textparser.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void textparser_validation_clear(textparser_validation *validation)
{
    validation_clear_internal(validation);
}

typedef enum {
    TEXTPARSER_TS_TARGET_INVALID = 0,
    TEXTPARSER_TS_TARGET_ASSIGNABLE = 1,
    TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN = 2,
} textparser_ts_target_state;

typedef struct textparser_typescript_label_frame {
    const textparser_node *name;
    bool iteration;
    unsigned function_depth;
    const struct textparser_typescript_label_frame *previous;
} textparser_typescript_label_frame;

typedef struct textparser_typescript_legality_context {
    unsigned function_depth;
    unsigned iteration_depth;
    unsigned switch_depth;
    bool async_function;
    bool generator_function;
    bool function_body;
    bool ambient;
    bool ambient_declare_allowed;
    const textparser_typescript_label_frame *labels;
} textparser_typescript_legality_context;

static const char *textparser_ts_production_name(textparser_t handle, const textparser_node *node)
{
    if (node == nullptr || (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) return nullptr;
    return textparser_grammar_node_name(handle, node);
}

static const char *textparser_ts_token_name(textparser_t handle, const textparser_node *node)
{
    if (node == nullptr || (node->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0) return nullptr;
    return textparser_grammar_node_name(handle, node);
}


static const textparser_node *textparser_typescript_node_token(
    const textparser_node *node,
    const char *token_name)
{
    if (node == nullptr || token_name == nullptr) return nullptr;
    for (const textparser_node *child = node->child; child != nullptr; child = child->next) {
        if ((child->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0 &&
            child->cst_kind != nullptr && strcmp(child->cst_kind, token_name) == 0)
            return child;
        const textparser_node *nested = textparser_typescript_node_token(child, token_name);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static const textparser_node *textparser_typescript_header_token(
    const textparser_node *node,
    const char *token_name)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->cst_kind != nullptr &&
            (strcmp(item->cst_kind, "BlockStatement") == 0 ||
             strcmp(item->cst_kind, "ClassBody") == 0 ||
             strcmp(item->cst_kind, "ArrowBody") == 0 ||
             strcmp(item->cst_kind, "BindingParameterList") == 0 ||
             strcmp(item->cst_kind, "TypeParametersContext") == 0 ||
             strcmp(item->cst_kind, "TypeAnnotation") == 0 ||
             strcmp(item->cst_kind, "Decorator") == 0 ||
             strcmp(item->cst_kind, "ClassMemberName") == 0 ||
             strcmp(item->cst_kind, "ObjectPropertyName") == 0))
            continue;
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0 &&
            item->cst_kind != nullptr && strcmp(item->cst_kind, token_name) == 0)
            return item;
        const textparser_node *nested = textparser_typescript_header_token(
            item->child, token_name);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static bool textparser_typescript_subtree_has_header_token(
    const textparser_node *node,
    const char *token_name)
{
    return textparser_typescript_header_token(node, token_name) != nullptr;
}

static bool textparser_typescript_same_identifier(
    textparser_t handle,
    const textparser_node *left,
    const textparser_node *right)
{
    if (handle == nullptr || left == nullptr || right == nullptr ||
        left->source_end < left->source_start || right->source_end < right->source_start)
        return false;
    size_t left_length = left->source_end - left->source_start;
    size_t right_length = right->source_end - right->source_start;
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    if (text == nullptr) return false;
    return left_length == right_length && left->source_end <= text_size &&
        right->source_end <= text_size &&
        memcmp(text + left->source_start,
            text + right->source_start, left_length) == 0;
}

static bool textparser_typescript_label_targets_iteration(const textparser_node *node)
{
    if (node == nullptr) return false;
    const char *kind = node->cst_kind;
    if (kind != nullptr &&
        (strcmp(kind, "IterationStatement") == 0 ||
         strcmp(kind, "WhileStatement") == 0 ||
         strcmp(kind, "DoStatement") == 0 ||
         strcmp(kind, "ForStatement") == 0))
        return true;
    if (kind != nullptr && strcmp(kind, "LabeledStatement") != 0 &&
        strcmp(kind, "Statement") != 0 && strncmp(kind, "Production", 10) != 0)
        return false;
    for (const textparser_node *child = node->child; child != nullptr; child = child->next)
        if ((child->node_flags & TEXTPARSER_NODE_SYNTHETIC) != 0 &&
            textparser_typescript_label_targets_iteration(child))
            return true;
    return false;
}

static const textparser_typescript_label_frame *textparser_typescript_find_label(
    textparser_t handle,
    const textparser_typescript_label_frame *labels,
    const textparser_node *name)
{
    for (const textparser_typescript_label_frame *label = labels;
         label != nullptr; label = label->previous)
        if (textparser_typescript_same_identifier(handle, label->name, name)) return label;
    return nullptr;
}

static const textparser_typescript_label_frame *textparser_typescript_find_local_label(
    textparser_t handle,
    const textparser_typescript_label_frame *labels,
    const textparser_node *name,
    unsigned function_depth)
{
    const textparser_typescript_label_frame *label =
        textparser_typescript_find_label(handle, labels, name);
    return label != nullptr && label->function_depth == function_depth ? label : nullptr;
}

static void textparser_typescript_report_node_diagnostic(
    textparser_t handle,
    const textparser_node *node,
    const char *code,
    const char *message)
{
    const textparser_node *terminal = textparser_node_first_terminal(node == nullptr
        ? nullptr : node->child);
    size_t start = terminal != nullptr ? terminal->source_start
        : node != nullptr ? node->source_start : 0;
    size_t end = terminal != nullptr ? terminal->source_end
        : node != nullptr ? node->source_end : start;
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, code, message,
        start, end >= start ? end - start : 0);
}

static bool textparser_typescript_is_header_boundary(const char *kind)
{
    return kind != nullptr &&
        (strcmp(kind, "BlockStatement") == 0 || strcmp(kind, "ArrowBody") == 0 ||
         strcmp(kind, "ClassBody") == 0 ||
         strcmp(kind, "BindingParameterList") == 0 ||
         strcmp(kind, "TypeParametersContext") == 0 ||
         strcmp(kind, "TypeAnnotation") == 0 || strcmp(kind, "Decorator") == 0 ||
         strcmp(kind, "ClassMemberName") == 0 ||
         strcmp(kind, "ObjectPropertyName") == 0);
}

static unsigned textparser_typescript_modifier_bit(const char *kind, bool *accessibility)
{
    *accessibility = false;
    if (kind == nullptr) return 0;
    if (strcmp(kind, "PublicKeyword") == 0) { *accessibility = true; return 1u << 0; }
    if (strcmp(kind, "PrivateKeyword") == 0) { *accessibility = true; return 1u << 1; }
    if (strcmp(kind, "ProtectedKeyword") == 0) { *accessibility = true; return 1u << 2; }
    if (strcmp(kind, "StaticKeyword") == 0) return 1u << 3;
    if (strcmp(kind, "ReadonlyKeyword") == 0) return 1u << 4;
    if (strcmp(kind, "AbstractKeyword") == 0) return 1u << 5;
    if (strcmp(kind, "OverrideKeyword") == 0) return 1u << 6;
    if (strcmp(kind, "DeclareKeyword") == 0) return 1u << 7;
    if (strcmp(kind, "AccessorKeyword") == 0) return 1u << 8;
    if (strcmp(kind, "AsyncKeyword") == 0) return 1u << 9;
    return 0;
}

static void textparser_typescript_report_modifier_diagnostic(
    textparser_t handle,
    const textparser_node *modifier,
    const char *code,
    const char *suffix)
{
    char spelling[24] = {0};
    size_t length = modifier->source_end - modifier->source_start;
    if (length >= sizeof(spelling)) length = sizeof(spelling) - 1;
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    if (text != nullptr && modifier->source_start + length <= text_size)
        memcpy(spelling, text + modifier->source_start, length);
    char message[128];
    snprintf(message, sizeof(message), "'%s' modifier %s", spelling, suffix);
    textparser_typescript_report_node_diagnostic(handle, modifier, code, message);
}

static void textparser_typescript_check_modifier_nodes(
    textparser_t handle,
    const textparser_node *node,
    unsigned *seen,
    bool *seen_accessibility)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (textparser_typescript_is_header_boundary(item->cst_kind)) continue;
        if ((item->node_flags & TEXTPARSER_NODE_SYNTHETIC) == 0) {
            bool accessibility = false;
            unsigned bit = textparser_typescript_modifier_bit(item->cst_kind, &accessibility);
            if (bit != 0) {
                if (accessibility && *seen_accessibility)
                    textparser_typescript_report_node_diagnostic(handle, item, "TS1028",
                        "Accessibility modifier already seen.");
                else if ((*seen & bit) != 0)
                    textparser_typescript_report_modifier_diagnostic(handle, item, "TS1030",
                        "already seen.");
                *seen |= bit;
                if (accessibility) *seen_accessibility = true;
            }
        }
        textparser_typescript_check_modifier_nodes(
            handle, item->child, seen, seen_accessibility);
    }
}

static size_t textparser_typescript_count_parameters_from_span(
    textparser_t handle,
    const textparser_node *parameter_list)
{
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    if (handle == nullptr || parameter_list == nullptr || text == nullptr ||
        parameter_list->source_end > text_size ||
        parameter_list->source_end <= parameter_list->source_start + 1) return 0;
    size_t start = parameter_list->source_start + 1;
    size_t end = parameter_list->source_end - 1;
    while (start < end && isspace((unsigned char)text[start])) start++;
    while (end > start && isspace((unsigned char)text[end - 1])) end--;
    if (start == end) return 0;
    size_t count = 1;
    unsigned round = 0, square = 0, brace = 0, angle = 0;
    for (size_t i = start; i < end; i++) {
        char c = text[i];
        if (c == '(') round++; else if (c == ')' && round != 0) round--;
        else if (c == '[') square++; else if (c == ']' && square != 0) square--;
        else if (c == '{') brace++; else if (c == '}' && brace != 0) brace--;
        else if (c == '<') angle++; else if (c == '>' && angle != 0) angle--;
        else if (c == ',' && round == 0 && square == 0 && brace == 0 && angle == 0) count++;
    }
    return count;
}

static bool textparser_typescript_preceded_by_declare(
    textparser_t handle,
    const textparser_node *node)
{
    const char *text = textparser_get_text(handle);
    if (handle == nullptr || node == nullptr || text == nullptr) return false;
    size_t start = node->source_start;
    while (start > 0 && text[start - 1] != '\n' &&
        text[start - 1] != '\r' && text[start - 1] != ';' &&
        text[start - 1] != '{' && text[start - 1] != '}') start--;
    static const char word[] = "declare";
    for (size_t i = start; i + sizeof(word) - 1 <= node->source_start; i++)
        if (memcmp(text + i, word, sizeof(word) - 1) == 0) return true;
    return false;
}

static bool textparser_typescript_header_has_accessor_word(
    textparser_t handle,
    const textparser_node *node,
    size_t end,
    const char *word)
{
    const char *text = textparser_get_text(handle);
    size_t text_size = textparser_get_text_size(handle);
    if (handle == nullptr || node == nullptr || text == nullptr || end > text_size) return false;
    size_t region_start = node->source_start;
    while (region_start > 0) {
        char previous = text[region_start - 1];
        if (previous == '\n' || previous == '\r' || previous == ';' || previous == '{' ||
            previous == '}') break;
        region_start--;
    }
    size_t word_length = strlen(word);
    for (size_t i = region_start; i + word_length <= end; i++) {
        if (memcmp(text + i, word, word_length) != 0) continue;
        bool left_boundary = i == region_start ||
            !(isalnum((unsigned char)text[i - 1]) ||
              text[i - 1] == '_' || text[i - 1] == '$');
        size_t after = i + word_length;
        bool right_boundary = after == end ||
            !(isalnum((unsigned char)text[after]) ||
              text[after] == '_' || text[after] == '$');
        if (!left_boundary || !right_boundary) continue;
        while (after < end && isspace((unsigned char)text[after])) after++;
        if (after < end && text[after] != '(') return true;
    }
    return false;
}

static const textparser_node *textparser_typescript_find_kind(
    const textparser_node *node,
    const char *kind)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->cst_kind != nullptr && strcmp(item->cst_kind, kind) == 0) return item;
        const textparser_node *nested = textparser_typescript_find_kind(item->child, kind);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static const textparser_node *textparser_typescript_find_kind_after(
    const textparser_node *node,
    const char *kind,
    size_t position)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        if (item->source_start >= position && item->cst_kind != nullptr &&
            strcmp(item->cst_kind, kind) == 0) return item;
        const textparser_node *nested = textparser_typescript_find_kind_after(
            item->child, kind, position);
        if (nested != nullptr) return nested;
    }
    return nullptr;
}

static bool textparser_typescript_is_ambient_statement(const char *kind)
{
    if (kind == nullptr) return false;
    static const char *statements[] = {
        "ExpressionStatement", "IfStatement", "WhileStatement", "DoStatement",
        "ForStatement", "ContinueStatement", "BreakStatement", "ReturnStatement",
        "ThrowStatement", "SwitchStatement", "TryStatement", "WithStatement",
        "DebuggerStatement", "LabeledStatement", nullptr
    };
    for (size_t i = 0; statements[i] != nullptr; i++)
        if (strcmp(kind, statements[i]) == 0) return true;
    return false;
}

static bool textparser_typescript_declaration_file(const textparser_t handle)
{
    const char *filename = textparser_get_filename(handle);
    if (filename == nullptr) return false;
    size_t length = strlen(filename);
#ifdef _WIN32
#define TEXTPARSER_DTS_SUFFIX(suffix) (length >= sizeof(suffix) - 1 && \
    _stricmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#else
#define TEXTPARSER_DTS_SUFFIX(suffix) (length >= sizeof(suffix) - 1 && \
    strcasecmp(filename + length - (sizeof(suffix) - 1), suffix) == 0)
#endif
    bool result = TEXTPARSER_DTS_SUFFIX(".d.ts") || TEXTPARSER_DTS_SUFFIX(".d.mts") ||
        TEXTPARSER_DTS_SUFFIX(".d.cts");
#undef TEXTPARSER_DTS_SUFFIX
    return result;
}

static bool textparser_typescript_has_declared_ancestor(const textparser_node *node)
{
    for (const textparser_node *parent = node == nullptr ? nullptr : node->parent;
         parent != nullptr; parent = parent->parent) {
        const char *kind = parent->cst_kind;
        if (kind != nullptr &&
            (strcmp(kind, "SourceFile") == 0 ||
             strcmp(kind, "Statement") == 0 ||
             strcmp(kind, "Repeat") == 0)) break;
        if (textparser_typescript_node_token(parent, "DeclareKeyword") != nullptr) return true;
    }
    return false;
}

static bool textparser_typescript_is_function_expression_node(const textparser_node *node)
{
    if (node == nullptr || node->cst_kind == nullptr ||
        strcmp(node->cst_kind, "BasePrimaryExpression") != 0)
        return false;
    for (const textparser_node *child = node->child; child != nullptr; child = child->next) {
        if (child->cst_kind != nullptr && strcmp(child->cst_kind, "FunctionKeyword") == 0)
            return true;
    }
    return false;
}

static void textparser_typescript_check_legality_nodes(
    textparser_t handle,
    const textparser_node *node,
    textparser_typescript_legality_context context)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *kind = item->cst_kind;
        textparser_typescript_legality_context child_context = context;
        bool declaration_with_modifiers = kind != nullptr &&
            (strcmp(kind, "DeclaredVariableStatement") == 0 ||
             strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0 ||
             strcmp(kind, "InterfaceDeclaration") == 0 ||
             strcmp(kind, "TypeAliasDeclaration") == 0 ||
             strcmp(kind, "EnumDeclaration") == 0 ||
             strcmp(kind, "NamespaceDeclaration") == 0 ||
             strcmp(kind, "GlobalDeclaration") == 0);
        const textparser_node *declare_modifier = declaration_with_modifiers
            ? textparser_typescript_header_token(item->child, "DeclareKeyword") : nullptr;
        bool ambient_declarator = kind != nullptr &&
            (strcmp(kind, "VariableDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0) &&
            (textparser_typescript_has_declared_ancestor(item) ||
             textparser_typescript_preceded_by_declare(handle, item));
        bool establishes_ambient = context.ambient || declare_modifier != nullptr ||
            ambient_declarator ||
            (kind != nullptr && strcmp(kind, "GlobalDeclaration") == 0);
        child_context.ambient = establishes_ambient;
        if (context.ambient && !context.ambient_declare_allowed && declare_modifier != nullptr)
            textparser_typescript_report_node_diagnostic(handle, declare_modifier, "TS1038",
                "A 'declare' modifier cannot be used in an already ambient context.");
        if (declare_modifier != nullptr || (kind != nullptr &&
            (strcmp(kind, "NamespaceDeclaration") == 0 ||
             strcmp(kind, "GlobalDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0)))
            child_context.ambient_declare_allowed = false;

        if (kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "ClassDeclaration") == 0 ||
             strcmp(kind, "DefaultClassDeclaration") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0 ||
             strcmp(kind, "BindingParameter") == 0)) {
            unsigned seen_modifiers = 0;
            bool seen_accessibility = false;
            textparser_typescript_check_modifier_nodes(
                handle, item->child, &seen_modifiers, &seen_accessibility);
        }

        if (kind != nullptr && strcmp(kind, "MethodDeclaration") == 0) {
            const textparser_node *readonly_modifier =
                textparser_typescript_header_token(item->child, "ReadonlyKeyword");
            const textparser_node *accessor_modifier =
                textparser_typescript_header_token(item->child, "AccessorKeyword");
            if (readonly_modifier != nullptr)
                textparser_typescript_report_node_diagnostic(handle, readonly_modifier, "TS1024",
                    "'readonly' modifier can only appear on a property declaration or index signature.");
            if (accessor_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, accessor_modifier, "TS1031",
                    "cannot appear on class elements of this kind.");
        } else if (kind != nullptr && strcmp(kind, "PropertyDeclaration") == 0) {
            const textparser_node *async_modifier =
                textparser_typescript_header_token(item->child, "AsyncKeyword");
            const textparser_node *readonly_modifier =
                textparser_typescript_header_token(item->child, "ReadonlyKeyword");
            const textparser_node *accessor_modifier =
                textparser_typescript_header_token(item->child, "AccessorKeyword");
            if (async_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, async_modifier, "TS1042",
                    "cannot be used here.");
            if (accessor_modifier != nullptr && readonly_modifier != nullptr)
                textparser_typescript_report_node_diagnostic(handle, accessor_modifier, "TS1243",
                    "'accessor' modifier cannot be used with 'readonly' modifier.");
        }

        if (kind != nullptr && strcmp(kind, "MethodDeclaration") == 0) {
            const textparser_node *parameter_list_node =
                textparser_typescript_find_kind(item->child, "BindingParameterList");
            size_t member_start = parameter_list_node == nullptr
                ? item->source_end : parameter_list_node->source_start;
            bool getter = textparser_typescript_header_has_accessor_word(
                handle, item, member_start, "get");
            bool setter = textparser_typescript_header_has_accessor_word(
                handle, item, member_start, "set");
            size_t parameters = textparser_typescript_count_parameters_from_span(
                handle, parameter_list_node);
            if (getter && parameters != 0)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1054",
                    "A 'get' accessor cannot have parameters.");
            if (setter && parameters != 1)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1049",
                    "A 'set' accessor must have exactly one parameter.");
            if ((getter || setter) && textparser_typescript_find_kind(
                    item->child, "TypeParametersContext") != nullptr)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1094",
                    "An accessor cannot have type parameters.");
            const textparser_node *parameter_list =
                textparser_typescript_find_kind(item->child, "BindingParameterList");
            if (setter && parameter_list != nullptr && textparser_typescript_find_kind_after(
                    item->child, "TypeAnnotation", parameter_list->source_end) != nullptr)
                textparser_typescript_report_node_diagnostic(handle, item, "TS1095",
                    "A 'set' accessor cannot have a return type annotation.");
            if (establishes_ambient && (getter || setter))
                textparser_typescript_report_node_diagnostic(handle, item, "TS1086",
                    "An accessor cannot be declared in an ambient context.");
        }

        if (establishes_ambient && kind != nullptr &&
            (strcmp(kind, "VariableDeclaration") == 0 ||
             strcmp(kind, "PropertyDeclaration") == 0)) {
            const textparser_node *initializer = textparser_typescript_find_kind(
                item->child, "Assign");
            if (initializer != nullptr)
                textparser_typescript_report_node_diagnostic(handle, initializer, "TS1039",
                    "Initializers are not allowed in ambient contexts.");
        }
        if (context.ambient && textparser_typescript_is_ambient_statement(kind))
            textparser_typescript_report_node_diagnostic(handle, item, "TS1036",
                "Statements are not allowed in ambient contexts.");

        bool implementation_declaration = kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0);
        if (establishes_ambient && implementation_declaration) {
            const textparser_node *body =
                textparser_typescript_find_kind(item->child, "BlockStatement");
            if (body != nullptr)
                textparser_typescript_report_node_diagnostic(handle, body, "TS1183",
                    "An implementation cannot be declared in ambient contexts.");
            const textparser_node *async_modifier =
                textparser_typescript_header_token(item->child, "AsyncKeyword");
            if (async_modifier != nullptr)
                textparser_typescript_report_modifier_diagnostic(handle, async_modifier, "TS1040",
                    "cannot be used in an ambient context.");
        }

        bool function_boundary = (kind != nullptr &&
            (strcmp(kind, "FunctionDeclaration") == 0 ||
             strcmp(kind, "DefaultFunctionDeclaration") == 0 ||
             strcmp(kind, "FunctionExpression") == 0 ||
             strcmp(kind, "ArrowFunction") == 0 ||
             strcmp(kind, "AsyncArrowFunction") == 0 ||
             strcmp(kind, "GenericArrowFunction") == 0 ||
             strcmp(kind, "ParenthesizedArrowFunction") == 0 ||
             strcmp(kind, "IdentifierArrowFunction") == 0 ||
             strcmp(kind, "MethodDeclaration") == 0 ||
             strcmp(kind, "ObjectMethodDeclaration") == 0 ||
             strcmp(kind, "ObjectAccessorDeclaration") == 0 ||
             strcmp(kind, "ConstructorDeclaration") == 0)) ||
            textparser_typescript_is_function_expression_node(item);
        bool static_block_boundary = kind != nullptr &&
            strcmp(kind, "ClassStaticBlock") == 0;
        if (static_block_boundary) {
            child_context.function_depth++;
            child_context.iteration_depth = 0;
            child_context.switch_depth = 0;
            child_context.async_function = false;
            child_context.generator_function = false;
            child_context.function_body = false;
            child_context.ambient = context.ambient;
        } else if (function_boundary) {
            child_context.function_depth++;
            child_context.iteration_depth = 0;
            child_context.switch_depth = 0;
            child_context.async_function =
                textparser_typescript_subtree_has_header_token(item->child, "AsyncKeyword");
            child_context.generator_function =
                textparser_typescript_subtree_has_header_token(item->child, "Multiply");
            child_context.function_body = true;
            if (establishes_ambient) child_context.ambient = false;
        } else if (kind != nullptr &&
            (strcmp(kind, "WhileStatement") == 0 ||
             strcmp(kind, "DoStatement") == 0 ||
             strcmp(kind, "ForStatement") == 0)) {
            child_context.iteration_depth++;
        } else if (kind != nullptr && strcmp(kind, "SwitchStatement") == 0) {
            child_context.switch_depth++;
        }

        textparser_typescript_label_frame label_frame = {0};
        if (kind != nullptr && strcmp(kind, "LabeledStatement") == 0) {
            label_frame.name = textparser_typescript_node_token(item, "Identifier");
            label_frame.iteration = textparser_typescript_label_targets_iteration(item);
            label_frame.function_depth = context.function_depth;
            label_frame.previous = context.labels;
            if (label_frame.name != nullptr && textparser_typescript_find_local_label(
                    handle, context.labels, label_frame.name, context.function_depth) != nullptr)
                textparser_typescript_report_node_diagnostic(handle, label_frame.name,
                    "TS1114", "Duplicate label.");
            child_context.labels = &label_frame;
        }

        const textparser_node *jump_label = kind != nullptr &&
            (strcmp(kind, "BreakStatement") == 0 || strcmp(kind, "ContinueStatement") == 0)
            ? textparser_typescript_node_token(item, "Identifier") : nullptr;
        const textparser_typescript_label_frame *target = jump_label == nullptr ? nullptr
            : textparser_typescript_find_label(handle, context.labels, jump_label);
        if (kind != nullptr && strcmp(kind, "ReturnStatement") == 0 &&
            !context.function_body) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1108",
                "A 'return' statement can only be used within a function body.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label == nullptr &&
            context.iteration_depth == 0 && context.switch_depth == 0) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1105",
                "A 'break' statement can only be used within an enclosing iteration or switch statement.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label == nullptr &&
            context.iteration_depth == 0) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1104",
                "A 'continue' statement can only be used within an enclosing iteration statement.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label != nullptr && target != nullptr &&
            target->function_depth != context.function_depth) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1107",
                "Jump target cannot cross function boundary.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label != nullptr && target != nullptr &&
            target->function_depth != context.function_depth) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1107",
                "Jump target cannot cross function boundary.");
        } else if (kind != nullptr && strcmp(kind, "BreakStatement") == 0 &&
            jump_label != nullptr && target == nullptr) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1116",
                "A 'break' statement can only jump to a label of an enclosing statement.");
        } else if (kind != nullptr && strcmp(kind, "ContinueStatement") == 0 &&
            jump_label != nullptr && (target == nullptr || !target->iteration)) {
            textparser_typescript_report_node_diagnostic(handle, jump_label, "TS1115",
                "A 'continue' statement can only jump to a label of an enclosing iteration statement.");
        } else if (kind != nullptr && strcmp(kind, "AwaitKeyword") == 0 &&
            context.function_depth != 0 && !context.async_function) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1308",
                "'await' expressions are only allowed within async functions and at the top levels of modules.");
        } else if (kind != nullptr && strcmp(kind, "YieldExpression") == 0 &&
            (context.function_depth == 0 || !context.generator_function)) {
            textparser_typescript_report_node_diagnostic(handle, item, "TS1163",
                "A 'yield' expression is only allowed in a generator body.");
        }
        textparser_typescript_check_legality_nodes(handle, item->child, child_context);
    }
}

static void textparser_typescript_check_legality(
    textparser_t handle,
    const textparser_node *root)
{
    textparser_typescript_legality_context context = {0};
    context.ambient = textparser_typescript_declaration_file(handle);
    context.ambient_declare_allowed = context.ambient;
    textparser_typescript_check_legality_nodes(handle, root, context);
}

/* Pratt target validation */

static const textparser_node *textparser_find_named_single_child(
    textparser_t handle,
    const textparser_node *node,
    const char **name)
{
    const textparser_node *current = node;
    while (current != nullptr) {
        const char *current_name = textparser_ts_production_name(handle, current);
        if (current_name != nullptr) {
            *name = current_name;
            return current;
        }
        if (current->child == nullptr || current->child->next != nullptr) break;
        current = current->child;
    }
    *name = nullptr;
    return current;
}

static textparser_ts_target_state textparser_typescript_assignment_target(
    textparser_t handle,
    const textparser_node *node,
    bool allow_pattern);

static bool textparser_typescript_pattern_value(
    textparser_t handle,
    const textparser_node *node)
{
    if (node == nullptr) return false;
    const char *token = textparser_ts_token_name(handle, node);
    if (token != nullptr && strcmp(token, "Assign") == 0 && node->child != nullptr)
        return textparser_typescript_assignment_target(handle, node->child, true) ==
            TEXTPARSER_TS_TARGET_ASSIGNABLE;
    return textparser_typescript_assignment_target(handle, node, true) ==
        TEXTPARSER_TS_TARGET_ASSIGNABLE;
}

static bool textparser_typescript_array_pattern_nodes_internal(
    textparser_t handle,
    const textparser_node *node,
    bool *seen_rest)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *token = textparser_ts_token_name(handle, item);
        if (*seen_rest && token != nullptr && strcmp(token, "Comma") == 0) return false;
        const char *name = textparser_ts_production_name(handle, item);
        if (name != nullptr && strcmp(name, "ArrayElement") == 0) {
            const textparser_node *value = textparser_node_last_child(item);
            const textparser_node *first = textparser_node_first_terminal(item->child);
            const char *first_name = textparser_ts_token_name(handle, first);
            bool rest = first_name != nullptr && strcmp(first_name, "Ellipsis") == 0;
            if (*seen_rest || (rest
                    ? textparser_typescript_assignment_target(handle, value, true) !=
                        TEXTPARSER_TS_TARGET_ASSIGNABLE
                    : !textparser_typescript_pattern_value(handle, value)))
                return false;
            *seen_rest = rest;
            continue;
        }
        if (!textparser_typescript_array_pattern_nodes_internal(
                handle, item->child, seen_rest)) return false;
    }
    return true;
}

static bool textparser_typescript_array_pattern_nodes(
    textparser_t handle,
    const textparser_node *node)
{
    bool seen_rest = false;
    return textparser_typescript_array_pattern_nodes_internal(handle, node, &seen_rest);
}

static bool textparser_typescript_object_pattern_element(
    textparser_t handle,
    const textparser_node *node,
    bool *is_rest)
{
    const char *name = nullptr;
    const textparser_node *element = textparser_find_named_single_child(handle, node, &name);
    if (element == nullptr || name == nullptr) return false;
    *is_rest = false;
    if (strcmp(name, "ObjectShorthandProperty") == 0) return true;
    if (strcmp(name, "ObjectPropertyAssignment") == 0)
        return textparser_typescript_pattern_value(handle, textparser_node_last_child(element));
    if (strcmp(name, "ObjectSpreadAssignment") == 0) {
        *is_rest = true;
        return textparser_typescript_assignment_target(
            handle, textparser_node_last_child(element), true) == TEXTPARSER_TS_TARGET_ASSIGNABLE;
    }
    return false;
}

static bool textparser_typescript_object_pattern_nodes_internal(
    textparser_t handle,
    const textparser_node *node,
    bool *seen_rest)
{
    for (const textparser_node *item = node; item != nullptr; item = item->next) {
        const char *token = textparser_ts_token_name(handle, item);
        if (*seen_rest && token != nullptr && strcmp(token, "Comma") == 0) return false;
        const char *name = textparser_ts_production_name(handle, item);
        if (name != nullptr && (strcmp(name, "ObjectShorthandProperty") == 0 ||
                strcmp(name, "ObjectPropertyAssignment") == 0 ||
                strcmp(name, "ObjectSpreadAssignment") == 0 ||
                strcmp(name, "ObjectMethodDeclaration") == 0 ||
                strcmp(name, "ObjectAccessorDeclaration") == 0)) {
            bool rest = false;
            if (*seen_rest || !textparser_typescript_object_pattern_element(handle, item, &rest))
                return false;
            *seen_rest = rest;
            continue;
        }
        if (!textparser_typescript_object_pattern_nodes_internal(
                handle, item->child, seen_rest)) return false;
    }
    return true;
}

static bool textparser_typescript_object_pattern_nodes(
    textparser_t handle,
    const textparser_node *node)
{
    bool seen_rest = false;
    return textparser_typescript_object_pattern_nodes_internal(handle, node, &seen_rest);
}

static textparser_ts_target_state textparser_typescript_assignment_target(
    textparser_t handle,
    const textparser_node *node,
    bool allow_pattern)
{
    if (node == nullptr) return TEXTPARSER_TS_TARGET_INVALID;
    const char *token = textparser_ts_token_name(handle, node);
    if (token != nullptr && strcmp(token, "LogicalNot") == 0 &&
        (node->node_flags & TEXTPARSER_NODE_GRAMMAR_POSTFIX) != 0)
        return textparser_typescript_assignment_target(handle, node->child, allow_pattern);
    if (token != nullptr)
        return strcmp(token, "Identifier") == 0
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;

    const char *name = textparser_ts_production_name(handle, node);
    const textparser_node *possible_suffix = node->child == nullptr
        ? nullptr : textparser_node_first_terminal(node->child->next);
    const char *possible_suffix_name = textparser_ts_token_name(handle, possible_suffix);
    bool is_postfix_node = possible_suffix_name != nullptr &&
        (strcmp(possible_suffix_name, "OptionalChain") == 0 ||
         strcmp(possible_suffix_name, "LParen") == 0 ||
         strcmp(possible_suffix_name, "LBracket") == 0 ||
         strcmp(possible_suffix_name, "Dot") == 0 ||
         strcmp(possible_suffix_name, "LogicalNot") == 0);
    if (is_postfix_node) {
        const textparser_node *left = node->child;
        const char *suffix = possible_suffix_name;
        if (suffix == nullptr) return TEXTPARSER_TS_TARGET_INVALID;
        if (strcmp(suffix, "OptionalChain") == 0) return TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN;
        if (strcmp(suffix, "LParen") == 0) return TEXTPARSER_TS_TARGET_INVALID;
        textparser_ts_target_state base = textparser_typescript_assignment_target(
            handle, left, allow_pattern);
        if (strcmp(suffix, "LogicalNot") == 0) return base;
        if (strcmp(suffix, "Dot") == 0 || strcmp(suffix, "LBracket") == 0)
            return base == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN
                ? TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN : TEXTPARSER_TS_TARGET_ASSIGNABLE;
        return TEXTPARSER_TS_TARGET_INVALID;
    }
    if (allow_pattern && name != nullptr && strcmp(name, "ArrayLiteralExpression") == 0)
        return textparser_typescript_array_pattern_nodes(handle, node->child)
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;
    if (allow_pattern && name != nullptr && strcmp(name, "ObjectLiteralBody") == 0)
        return textparser_typescript_object_pattern_nodes(handle, node->child)
            ? TEXTPARSER_TS_TARGET_ASSIGNABLE : TEXTPARSER_TS_TARGET_INVALID;

    if (node->child != nullptr && node->child->next == nullptr)
        return textparser_typescript_assignment_target(handle, node->child, allow_pattern);

    const textparser_node *first = textparser_node_first_terminal(node->child);
    const textparser_node *last_child = textparser_node_last_child(node);
    const textparser_node *last = textparser_node_first_terminal(last_child);
    const char *first_name = textparser_ts_token_name(handle, first);
    const char *last_name = textparser_ts_token_name(handle, last);
    if (first_name != nullptr && last_name != nullptr &&
        strcmp(first_name, "LParen") == 0 && strcmp(last_name, "RParen") == 0) {
        const textparser_node *middle = node->child == nullptr ? nullptr : node->child->next;
        textparser_ts_target_state inner = textparser_typescript_assignment_target(
            handle, middle, allow_pattern);
        return inner == TEXTPARSER_TS_TARGET_ASSIGNABLE
            ? inner : TEXTPARSER_TS_TARGET_INVALID;
    }
    return TEXTPARSER_TS_TARGET_INVALID;
}

static bool typescript_operand_validator(
    textparser_t handle,
    const char *validator_name,
    const textparser_node *node,
    bool allow_pattern,
    const char **out_code,
    const char **out_message,
    size_t *out_start,
    size_t *out_length,
    void *user_data)
{
    (void)user_data;
    if (validator_name == nullptr) return true;
    bool assignment = strcmp(validator_name, "typescript.assignmentTarget") == 0;
    bool update = strcmp(validator_name, "typescript.updateTarget") == 0;
    if (!assignment && !update) return true;

    textparser_ts_target_state state = textparser_typescript_assignment_target(
        handle, node, allow_pattern);
    if (state == TEXTPARSER_TS_TARGET_ASSIGNABLE) return true;

    size_t start = node != nullptr ? node->source_start : 0;
    size_t end = node != nullptr ? node->source_end : start;
    if (out_code != nullptr) {
        *out_code = update ? "TS2357" :
            state == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN ? "TS2779" : "TS2364";
    }
    if (out_message != nullptr) {
        *out_message = update
            ? "The operand of an increment or decrement operator must be a variable or a property access."
            : state == TEXTPARSER_TS_TARGET_OPTIONAL_CHAIN
                ? "The left-hand side of an assignment expression may not be an optional property access."
                : "The left-hand side of an assignment expression must be a variable or a property access.";
    }
    if (out_start != nullptr) *out_start = start;
    if (out_length != nullptr) *out_length = end >= start ? end - start : 0;

    return false;
}

static textparser_action typescript_source_complete_handler(
    textparser_t handle,
    const textparser_event *event,
    void *user_data)
{
    (void)user_data;
    if (event != nullptr && event->node != nullptr) {
        textparser_typescript_check_legality(handle, event->node);
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

static bool typescript_identifier_validator(
    textparser_t handle,
    const char *text,
    size_t length,
    const char **out_error,
    void *user_data)
{
    (void)user_data;
    if (handle == nullptr || text == nullptr || length == 0) return false;
    if (memchr(text, '\\', length) == nullptr) return true;

    char *decoded = malloc(length + 1);
    if (decoded == nullptr) return false;
    size_t i = text[0] == '#' ? 1 : 0;
    size_t decoded_length = 0;
    while (i < length) {
        if (text[i] != '\\') {
            unsigned char ch = (unsigned char)text[i];
            size_t width = ch < 0x80 ? 1 :
                ((ch & 0xe0) == 0xc0 ? 2 : ((ch & 0xf0) == 0xe0 ? 3 : 4));
            if (i + width > length) { free(decoded); return false; }
            memcpy(decoded + decoded_length, text + i, width);
            decoded_length += width;
            i += width;
            continue;
        }
        if (i + 2 >= length || text[i + 1] != 'u') { free(decoded); return false; }
        i += 2;
        bool braced = i < length && text[i] == '{';
        if (braced) i++;
        uint32_t value = 0;
        size_t digits = 0;
        while (i < length && digits < (braced ? 6u : 4u)) {
            unsigned char ch = (unsigned char)text[i];
            unsigned digit;
            if (ch >= '0' && ch <= '9') digit = ch - '0';
            else if (ch >= 'a' && ch <= 'f') digit = ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') digit = ch - 'A' + 10;
            else break;
            value = value * 16 + digit;
            i++; digits++;
        }
        if ((!braced && digits != 4) || (braced &&
            (digits == 0 || i >= length || text[i++] != '}')) ||
            value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) {
            if (out_error != nullptr) *out_error = "Invalid Unicode escape sequence in identifier";
            free(decoded);
            return false;
        }
        if (value <= 0x7f) decoded[decoded_length++] = (char)value;
        else if (value <= 0x7ff) {
            decoded[decoded_length++] = (char)(0xc0 | (value >> 6));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        } else if (value <= 0xffff) {
            decoded[decoded_length++] = (char)(0xe0 | (value >> 12));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 6) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        } else {
            decoded[decoded_length++] = (char)(0xf0 | (value >> 18));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 12) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | ((value >> 6) & 0x3f));
            decoded[decoded_length++] = (char)(0x80 | (value & 0x3f));
        }
    }
    decoded[decoded_length] = '\0';
    static const char *identifier_pattern =
        "(?:[$_]|\\p{ID_Start})(?:[$\\x{200C}\\x{200D}]|\\p{ID_Continue})*";
    size_t found_at = 0, found_length = 0;
    bool valid = textparser_regex_match_pattern(
        handle, identifier_pattern, decoded, decoded_length, true, &found_at, &found_length) &&
        found_at == 0 && found_length == decoded_length;
    if (!valid && out_error != nullptr) {
        *out_error = "Unicode escape does not produce a valid identifier character";
    }
    free(decoded);
    return valid;
}

EXPORT_TYPESCRIPT int textparser_typescript_register_validators(textparser_t handle)
{
    if (handle == nullptr) return -1;
    int err = 0;
    err |= textparser_register_validator(
        handle, "typescript.identifier", typescript_identifier_validator, nullptr);
    err |= textparser_register_operand_validator(
        handle, "typescript.assignmentTarget", typescript_operand_validator, nullptr);
    err |= textparser_register_operand_validator(
        handle, "typescript.updateTarget", typescript_operand_validator, nullptr);
    err |= textparser_register_handler(
        handle, "typescript.legality", typescript_source_complete_handler, nullptr);
    err |= textparser_register_handler(
        handle, "source.complete", typescript_source_complete_handler, nullptr);
    return err;
}

EXPORT_TYPESCRIPT textparser_validation *textparser_validate_typescript(textparser_t handle)
{
    if (handle == nullptr) return nullptr;
    textparser_validation *validation = nullptr;
    size_t diag_count = textparser_get_diagnostic_count(handle);
    for (size_t i = 0; i < diag_count; i++) {
        textparser_diagnostic diag = {0};
        if (textparser_get_diagnostic(handle, i, &diag) == 0) {
            enum textparser_validation_item_type type = TEXTPARSER_VALIDATION_ITEM_TYPE_INFO;
            if (diag.severity == TEXTPARSER_SEVERITY_ERROR)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR;
            else if (diag.severity == TEXTPARSER_SEVERITY_WARNING)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING;

            char *msg = dynamic_printf("%s: %s", diag.code ? diag.code : "TS", diag.message ? diag.message : "");
            textparser_validation_item_add(type, &validation, msg, diag.start_pos, diag.length);
        }
    }
    return validation;
}
