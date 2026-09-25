#include "asm.h"
#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOLS 1024

typedef struct {
    char name[128];
    uint32_t start_pos;
} asm_symbol;

typedef struct {
    asm_symbol labels[MAX_SYMBOLS];
    size_t label_count;
    asm_symbol procs[MAX_SYMBOLS];
    size_t proc_count;
} asm_context;

void textparser_validation_clear(textparser_validation *validation) {
    validation_clear_internal(validation);
}

static inline bool is_node(textparser_t handle, const textparser_node *node, const char *name) {
    if (!node) return false;
    const char *n = textparser_grammar_node_name(handle, node);
    return n != NULL && strcmp(n, name) == 0;
}

static char *node_text(textparser_t handle, const textparser_node *node, size_t *length) {
    if (!node || node->source_end < node->source_start) return NULL;
    textparser_token_item prefix = {.len = node->source_start};
    textparser_token_item slice = {.prev = &prefix, .len = node->source_end - node->source_start};
    *length = slice.len;
    uint16_t *wide16 = textparser_get_token_text16(handle, &slice);
    uint32_t *wide32 = wide16 ? NULL : textparser_get_token_text32(handle, &slice);
    if (wide16 || wide32) {
        *length = 0;
        for (size_t i = 0; i < slice.len; ++i) {
            uint32_t cp = wide16 ? wide16[i] : wide32[i];
            if (wide16 && cp >= 0xd800 && cp <= 0xdbff && i + 1 < slice.len &&
                wide16[i + 1] >= 0xdc00 && wide16[i + 1] <= 0xdfff) {
                cp = 0x10000 + ((cp - 0xd800) << 10) + wide16[++i] - 0xdc00;
            }
            *length += cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
        }
    }
    textparser_free_token_text(wide16);
    textparser_free_token_text(wide32);
    return textparser_get_token_text(handle, &slice);
}

static void trim_string(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

static void report(textparser_t handle, const textparser_node *node,
                   const char *code, const char *message) {
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_ERROR, code, message,
        node->source_start, node->source_end - node->source_start);
}

static void report_warning(textparser_t handle, const textparser_node *node,
                           const char *code, const char *message) {
    textparser_report_diagnostic(handle, TEXTPARSER_SEVERITY_WARNING, code, message,
        node->source_start, node->source_end - node->source_start);
}

static const textparser_node *find_descendant_node(textparser_t handle, const textparser_node *node, const char *name) {
    if (!node) return NULL;
    if (is_node(handle, node, name)) return node;
    for (const textparser_node *c = node->child; c; c = c->next) {
        const textparser_node *found = find_descendant_node(handle, c, name);
        if (found) return found;
    }
    return NULL;
}

static bool is_operand_node(textparser_t handle, const textparser_node *node) {
    if (!node) return false;
    return is_node(handle, node, "Operand") ||
           is_node(handle, node, "RegisterOperand") ||
           is_node(handle, node, "MemoryOperand") ||
           is_node(handle, node, "ImmediateOperand") ||
           is_node(handle, node, "ARMRegisterList") ||
           is_node(handle, node, "ARMShiftedOperand") ||
           is_node(handle, node, "QualifiedOperand") ||
           is_node(handle, node, "BracketedMemory") ||
           is_node(handle, node, "AttMemory");
}

static const textparser_node *find_reg_token(textparser_t handle, const textparser_node *operand) {
    if (!operand) return NULL;
    if (is_node(handle, operand, "Register") ||
        is_node(handle, operand, "ArmBranchMnemonic") ||
        is_node(handle, operand, "AttRegister") ||
        is_node(handle, operand, "X86SegmentRegister")) {
        return operand;
    }
    for (const textparser_node *c = operand->child; c; c = c->next) {
        const textparser_node *found = find_reg_token(handle, c);
        if (found) return found;
    }
    return NULL;
}

static int get_x86_register_size(const char *reg_name) {
    if (!reg_name) return 0;
    char lower[32];
    size_t i = 0;
    while (reg_name[i] && i < 31 && !isspace((unsigned char)reg_name[i])) {
        lower[i] = (char)tolower((unsigned char)reg_name[i]);
        i++;
    }
    lower[i] = '\0';

    // 64-bit registers
    if (strcmp(lower, "rax") == 0 || strcmp(lower, "rbx") == 0 ||
        strcmp(lower, "rcx") == 0 || strcmp(lower, "rdx") == 0 ||
        strcmp(lower, "rsi") == 0 || strcmp(lower, "rdi") == 0 ||
        strcmp(lower, "rbp") == 0 || strcmp(lower, "rsp") == 0 ||
        strcmp(lower, "rip") == 0) {
        return 64;
    }
    if (lower[0] == 'r') {
        char *endptr = NULL;
        long n = strtol(lower + 1, &endptr, 10);
        if (n >= 8 && n <= 15) {
            if (*endptr == '\0') return 64;
            if (*endptr == 'd' && *(endptr + 1) == '\0') return 32;
            if (*endptr == 'w' && *(endptr + 1) == '\0') return 16;
            if (*endptr == 'b' && *(endptr + 1) == '\0') return 8;
        }
    }

    // 32-bit registers
    if (strcmp(lower, "eax") == 0 || strcmp(lower, "ebx") == 0 ||
        strcmp(lower, "ecx") == 0 || strcmp(lower, "edx") == 0 ||
        strcmp(lower, "esi") == 0 || strcmp(lower, "edi") == 0 ||
        strcmp(lower, "ebp") == 0 || strcmp(lower, "esp") == 0 ||
        strcmp(lower, "eip") == 0) {
        return 32;
    }

    // 16-bit registers
    if (strcmp(lower, "ax") == 0 || strcmp(lower, "bx") == 0 ||
        strcmp(lower, "cx") == 0 || strcmp(lower, "dx") == 0 ||
        strcmp(lower, "si") == 0 || strcmp(lower, "di") == 0 ||
        strcmp(lower, "bp") == 0 || strcmp(lower, "sp") == 0 ||
        strcmp(lower, "ip") == 0) {
        return 16;
    }

    // 8-bit registers
    if (strcmp(lower, "al") == 0 || strcmp(lower, "bl") == 0 ||
        strcmp(lower, "cl") == 0 || strcmp(lower, "dl") == 0 ||
        strcmp(lower, "ah") == 0 || strcmp(lower, "bh") == 0 ||
        strcmp(lower, "ch") == 0 || strcmp(lower, "dh") == 0 ||
        strcmp(lower, "sil") == 0 || strcmp(lower, "dil") == 0 ||
        strcmp(lower, "bpl") == 0 || strcmp(lower, "spl") == 0) {
        return 8;
    }

    return 0;
}

static void get_mnemonic_lower(const char *m, char *out, size_t max_len) {
    if (!m) { out[0] = '\0'; return; }
    size_t i = 0;
    while (*m && isspace((unsigned char)*m)) m++;
    while (*m && i < max_len - 1 && !isspace((unsigned char)*m)) {
        out[i] = (char)tolower((unsigned char)*m);
        i++;
        m++;
    }
    out[i] = '\0';
}

static bool is_nullary_instruction(const char *m) {
    return strcmp(m, "nop") == 0 ||
           strcmp(m, "syscall") == 0 ||
           strcmp(m, "sysret") == 0 ||
           strcmp(m, "sysenter") == 0 ||
           strcmp(m, "sysexit") == 0 ||
           strcmp(m, "cpuid") == 0 ||
           strcmp(m, "hlt") == 0 ||
           strcmp(m, "pause") == 0 ||
           strcmp(m, "cli") == 0 ||
           strcmp(m, "sti") == 0 ||
           strcmp(m, "cld") == 0 ||
           strcmp(m, "std") == 0 ||
           strcmp(m, "clc") == 0 ||
           strcmp(m, "stc") == 0 ||
           strcmp(m, "cmc") == 0 ||
           strcmp(m, "cdq") == 0 ||
           strcmp(m, "cqo") == 0 ||
           strcmp(m, "cbw") == 0 ||
           strcmp(m, "cwd") == 0 ||
           strcmp(m, "cwde") == 0 ||
           strcmp(m, "cdqe") == 0 ||
           strcmp(m, "rdtsc") == 0 ||
           strcmp(m, "rdtscp") == 0 ||
           strcmp(m, "ud2") == 0 ||
           strcmp(m, "wait") == 0 ||
           strcmp(m, "fwait") == 0 ||
           strcmp(m, "fnop") == 0;
}

static bool is_unary_instruction(const char *m) {
    return strcmp(m, "push") == 0 ||
           strcmp(m, "pop") == 0 ||
           strcmp(m, "inc") == 0 ||
           strcmp(m, "dec") == 0 ||
           strcmp(m, "not") == 0 ||
           strcmp(m, "neg") == 0 ||
           strcmp(m, "jmp") == 0 ||
           strcmp(m, "call") == 0 ||
           strcmp(m, "int") == 0;
}

static bool is_binary_instruction(const char *m) {
    return strcmp(m, "mov") == 0 ||
           strcmp(m, "lea") == 0 ||
           strcmp(m, "xchg") == 0;
}

static bool is_lockable_instruction(const char *m) {
    return strcmp(m, "add") == 0 ||
           strcmp(m, "adc") == 0 ||
           strcmp(m, "and") == 0 ||
           strcmp(m, "btc") == 0 ||
           strcmp(m, "btr") == 0 ||
           strcmp(m, "bts") == 0 ||
           strcmp(m, "cmpxchg") == 0 ||
           strcmp(m, "dec") == 0 ||
           strcmp(m, "inc") == 0 ||
           strcmp(m, "neg") == 0 ||
           strcmp(m, "not") == 0 ||
           strcmp(m, "or") == 0 ||
           strcmp(m, "sbb") == 0 ||
           strcmp(m, "sub") == 0 ||
           strcmp(m, "xor") == 0 ||
           strcmp(m, "xadd") == 0 ||
           strcmp(m, "xchg") == 0;
}

static bool is_reg_size_checked_instruction(const char *m) {
    return strcmp(m, "mov") == 0 ||
           strcmp(m, "add") == 0 ||
           strcmp(m, "sub") == 0 ||
           strcmp(m, "cmp") == 0 ||
           strcmp(m, "test") == 0 ||
           strcmp(m, "xor") == 0 ||
           strcmp(m, "and") == 0 ||
           strcmp(m, "or") == 0;
}

static const textparser_node *find_mnemonic_node(textparser_t handle, const textparser_node *instr) {
    for (const textparser_node *c = instr->child; c; c = c->next) {
        if (is_node(handle, c, "InstructionPrefix") || is_node(handle, c, "Repeat")) continue;
        if (is_node(handle, c, "OperandList") || is_operand_node(handle, c)) continue;
        return c;
    }
    return NULL;
}

static const textparser_node *find_operand_list(textparser_t handle, const textparser_node *instr) {
    for (const textparser_node *c = instr->child; c; c = c->next) {
        if (is_node(handle, c, "OperandList")) return c;
        if (is_operand_node(handle, c)) return instr;
    }
    return NULL;
}

static void collect_operands_recursive(textparser_t handle, const textparser_node *node,
                                       const textparser_node **operands, size_t *count, size_t max_operands) {
    if (!node || *count >= max_operands) return;
    if (is_operand_node(handle, node)) {
        operands[(*count)++] = node;
        return;
    }
    for (const textparser_node *c = node->child; c; c = c->next) {
        collect_operands_recursive(handle, c, operands, count, max_operands);
    }
}

static size_t collect_operands(textparser_t handle, const textparser_node *instr,
                               const textparser_node **operands, size_t max_operands) {
    const textparser_node *op_list = find_operand_list(handle, instr);
    if (!op_list) return 0;
    size_t count = 0;
    // If op_list is instr itself, iterate over operand children
    if (op_list == instr) {
        for (const textparser_node *c = instr->child; c; c = c->next) {
            if (is_operand_node(handle, c)) {
                if (count < max_operands) operands[count] = c;
                count++;
            }
        }
        return count;
    }
    collect_operands_recursive(handle, op_list, operands, &count, max_operands);
    return count;
}

static void count_prefixes_recursive(textparser_t handle, const textparser_node *node,
                                     int *lock_count, int *rep_count, int *seg_count) {
    if (!node) return;
    if (is_node(handle, node, "InstructionPrefix")) {
        if (find_descendant_node(handle, node, "LockPrefix")) (*lock_count)++;
        if (find_descendant_node(handle, node, "RepPrefix")) (*rep_count)++;
        if (find_descendant_node(handle, node, "X86SegmentRegister")) (*seg_count)++;
        return;
    }
    if (is_node(handle, node, "OperandList") ||
        is_node(handle, node, "InstructionMnemonic") ||
        is_node(handle, node, "ArmBranchCond") ||
        is_node(handle, node, "ArmBranchMnemonic") ||
        is_operand_node(handle, node)) {
        return;
    }
    for (const textparser_node *c = node->child; c; c = c->next) {
        count_prefixes_recursive(handle, c, lock_count, rep_count, seg_count);
    }
}

static void check_label_definition(textparser_t handle, const textparser_node *node, asm_context *ctx) {
    const textparser_node *label_node = node->child;
    if (label_node && is_node(handle, label_node, "Colon")) {
        label_node = NULL;
    }
    if (!label_node) return;

    size_t len = 0;
    char *text = node_text(handle, label_node, &len);
    if (!text) return;
    trim_string(text);

    // Strip trailing colon if present
    size_t slen = strlen(text);
    if (slen > 0 && text[slen - 1] == ':') {
        text[slen - 1] = '\0';
        trim_string(text);
        slen = strlen(text);
    }

    if (slen == 0) {
        free(text);
        return;
    }

    // Skip numeric labels (e.g. 1:, 2:) as they can be reused locally in GAS
    bool is_all_digits = true;
    for (size_t i = 0; i < slen; i++) {
        if (!isdigit((unsigned char)text[i])) {
            is_all_digits = false;
            break;
        }
    }

    // Skip local labels starting with '.'
    if (text[0] == '.' || is_all_digits) {
        free(text);
        return;
    }

    // Check duplicate
    for (size_t i = 0; i < ctx->label_count; i++) {
        if (strcmp(ctx->labels[i].name, text) == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Duplicate label '%s'.", text);
            report(handle, label_node, "ASM2001", msg);
            free(text);
            return;
        }
    }

    if (ctx->label_count < MAX_SYMBOLS) {
        snprintf(ctx->labels[ctx->label_count].name, sizeof(ctx->labels[0].name), "%s", text);
        ctx->labels[ctx->label_count].start_pos = label_node->source_start;
        ctx->label_count++;
    }
    free(text);
}

static const textparser_node *find_proc_identifier(textparser_t handle, const textparser_node *proc_node) {
    for (const textparser_node *c = proc_node->child; c; c = c->next) {
        if (is_node(handle, c, "Identifier") || is_node(handle, c, "LabelName")) return c;
        if (c->child) {
            for (const textparser_node *cc = c->child; cc; cc = cc->next) {
                if (is_node(handle, cc, "Identifier") || is_node(handle, cc, "LabelName")) return cc;
            }
        }
    }
    return NULL;
}

static void check_procedure_definition(textparser_t handle, const textparser_node *node, asm_context *ctx) {
    const textparser_node *name_node = find_proc_identifier(handle, node);
    if (!name_node) return;

    size_t len = 0;
    char *text = node_text(handle, name_node, &len);
    if (!text) return;
    trim_string(text);

    if (strlen(text) == 0) {
        free(text);
        return;
    }

    for (size_t i = 0; i < ctx->proc_count; i++) {
        if (strcmp(ctx->procs[i].name, text) == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Duplicate procedure definition '%s'.", text);
            report(handle, name_node, "ASM2006", msg);
            free(text);
            return;
        }
    }

    if (ctx->proc_count < MAX_SYMBOLS) {
        snprintf(ctx->procs[ctx->proc_count].name, sizeof(ctx->procs[0].name), "%s", text);
        ctx->procs[ctx->proc_count].start_pos = name_node->source_start;
        ctx->proc_count++;
    }
    free(text);
}

static void check_instruction_statement(textparser_t handle, const textparser_node *node) {
    const textparser_node *mnem_node = find_mnemonic_node(handle, node);
    if (!mnem_node) return;

    size_t mlen = 0;
    char *raw_mnem = node_text(handle, mnem_node, &mlen);
    if (!raw_mnem) return;

    char mnem[64];
    get_mnemonic_lower(raw_mnem, mnem, sizeof(mnem));
    free(raw_mnem);

    if (strlen(mnem) == 0) return;

    // Check prefixes
    int lock_count = 0;
    int rep_count = 0;
    int seg_count = 0;
    count_prefixes_recursive(handle, node, &lock_count, &rep_count, &seg_count);

    if (lock_count > 1) {
        report(handle, node, "ASM2004", "Duplicate instruction prefix 'lock'.");
    }
    if (rep_count > 1) {
        report(handle, node, "ASM2004", "Duplicate instruction prefix 'rep'.");
    }
    if (seg_count > 1) {
        report(handle, node, "ASM2004", "Conflicting segment override prefixes.");
    }
    if (lock_count > 0 && !is_lockable_instruction(mnem)) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Invalid 'lock' prefix on instruction '%s' (lock prefix is only valid on read-modify-write operations).",
                 mnem);
        report(handle, node, "ASM2004", msg);
    }

    // Check operands
    const textparser_node *operands[8] = {NULL};
    size_t op_count = collect_operands(handle, node, operands, 8);

    if (is_nullary_instruction(mnem)) {
        if (op_count > 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Instruction '%s' expects 0 operands, but got %zu.",
                     mnem, op_count);
            report(handle, node, "ASM2002", msg);
        }
    } else if (is_unary_instruction(mnem)) {
        if (op_count == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Instruction '%s' requires at least 1 operand.", mnem);
            report(handle, node, "ASM2002", msg);
        }
    } else if (is_binary_instruction(mnem)) {
        if (op_count < 2) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Instruction '%s' requires 2 operands, but got %zu.",
                     mnem, op_count);
            report(handle, node, "ASM2002", msg);
        }
    }

    // Check ambiguous memory operand size with immediate (ASM2003)
    if (op_count == 2 && operands[0] && operands[1]) {
        bool op0_has_lbracket = (find_descendant_node(handle, operands[0], "LBracket") != NULL);
        bool op0_has_size_spec = (find_descendant_node(handle, operands[0], "SizeSpecifier") != NULL);
        bool op1_has_reg = (find_reg_token(handle, operands[1]) != NULL);
        bool op1_has_lbracket = (find_descendant_node(handle, operands[1], "LBracket") != NULL);

        if (op0_has_lbracket && !op0_has_size_spec && !op1_has_reg && !op1_has_lbracket) {
            report_warning(handle, operands[0], "ASM2003",
                           "Missing size specifier (e.g. 'byte ptr', 'dword ptr') for ambiguous memory operand with immediate value.");
        }

        // Check register size mismatch (ASM2005)
        if (is_reg_size_checked_instruction(mnem) && !op0_has_lbracket && !op1_has_lbracket) {
            const textparser_node *reg_node0 = find_reg_token(handle, operands[0]);
            const textparser_node *reg_node1 = find_reg_token(handle, operands[1]);
            if (reg_node0 && reg_node1) {
                size_t len0 = 0, len1 = 0;
                char *r0_text = node_text(handle, reg_node0, &len0);
                char *r1_text = node_text(handle, reg_node1, &len1);
                if (r0_text && r1_text) {
                    trim_string(r0_text);
                    trim_string(r1_text);
                    int s0 = get_x86_register_size(r0_text);
                    int s1 = get_x86_register_size(r1_text);
                    if (s0 > 0 && s1 > 0 && s0 != s1) {
                        char msg[256];
                        snprintf(msg, sizeof(msg),
                                 "Operand size mismatch between register '%s' (%d-bit) and '%s' (%d-bit).",
                                 r0_text, s0, r1_text, s1);
                        report(handle, node, "ASM2005", msg);
                    }
                }
                free(r0_text);
                free(r1_text);
            }
        }
    }
}

static void check_asm_node(textparser_t handle, const textparser_node *node, asm_context *ctx) {
    if (!node) return;

    if (is_node(handle, node, "LabelDefinition")) {
        check_label_definition(handle, node, ctx);
    } else if (is_node(handle, node, "ProcedureDefinition")) {
        check_procedure_definition(handle, node, ctx);
    } else if (is_node(handle, node, "InstructionStatement")) {
        check_instruction_statement(handle, node);
    }

    for (const textparser_node *child = node->child; child; child = child->next) {
        check_asm_node(handle, child, ctx);
    }
}

static void validate_asm_ast(textparser_t handle, const textparser_node *root) {
    asm_context ctx = {0};
    check_asm_node(handle, root, &ctx);
}

static textparser_action asm_source_complete(textparser_t handle,
                                             const textparser_event *event,
                                             void *user_data) {
    (void)user_data;
    if (event && event->node) {
        for (const textparser_node *item = event->node; item; item = item->next) {
            validate_asm_ast(handle, item);
        }
    }
    return TEXTPARSER_ACTION_ACCEPT;
}

EXPORT_ASM int textparser_asm_register_validators(textparser_t handle) {
    if (!handle) return -1;
    return textparser_register_handler(handle, "asm.legality", asm_source_complete, NULL);
}

EXPORT_ASM textparser_validation *textparser_validate_asm(textparser_t handle) {
    if (handle == NULL) return NULL;

    size_t diag_count = textparser_get_diagnostic_count(handle);
    if (diag_count == 0) {
        const textparser_node *root = textparser_get_first_token(handle);
        if (root) {
            validate_asm_ast(handle, root);
            diag_count = textparser_get_diagnostic_count(handle);
        }
    }

    if (diag_count == 0) return NULL;

    textparser_validation *validation = NULL;
    for (size_t i = 0; i < diag_count; i++) {
        textparser_diagnostic diag = {0};
        if (textparser_get_diagnostic(handle, i, &diag) == 0) {
            enum textparser_validation_item_type type = TEXTPARSER_VALIDATION_ITEM_TYPE_INFO;
            if (diag.severity == TEXTPARSER_SEVERITY_ERROR)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR;
            else if (diag.severity == TEXTPARSER_SEVERITY_WARNING)
                type = TEXTPARSER_VALIDATION_ITEM_TYPE_WARNING;

            char *msg = dynamic_printf("%s: %s",
                                       diag.code ? diag.code : "ASM",
                                       diag.message ? diag.message : "");
            textparser_validation_item_add(type, &validation, msg, diag.start_pos, diag.length);
        }
    }
    return validation;
}
