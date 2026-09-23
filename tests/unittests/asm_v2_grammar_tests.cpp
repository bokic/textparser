#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <asm_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct AsmGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/asm_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &asm_definition;
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
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, AsmGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Program ---

TEST_P(AsmGrammarFixture, parses_basic_x86_program) {
    const char *source =
        "; x86-64 assembly example\n"
        "section .data\n"
        "    msg db 'Hello World', 0\n"
        "    len equ $ - msg\n"
        "\n"
        "section .text\n"
        "    global _start\n"
        "\n"
        "_start:\n"
        "    ; write syscall\n"
        "    mov rax, 1\n"
        "    mov rdi, 1\n"
        "    mov rsi, msg\n"
        "    mov rdx, len\n"
        "    syscall\n"
        "\n"
        "    ; exit syscall\n"
        "    mov rax, 60\n"
        "    xor rdi, rdi\n"
        "    syscall\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DirectiveStatement"), nullptr);
    EXPECT_NE(find(node, "DataDefinition"), nullptr);
    EXPECT_NE(find(node, "EquateStatement"), nullptr);
    EXPECT_NE(find(node, "LabelDefinition"), nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
}

// --- Addressing Modes ---

TEST_P(AsmGrammarFixture, parses_x86_addressing_modes) {
    const char *source =
        "mov eax, [ebx]\n"
        "mov eax, [ebx + 4]\n"
        "mov eax, [ebx + ecx*4]\n"
        "mov eax, [ebx + ecx*4 + 16]\n"
        "mov dword ptr [rbp - 8], 42\n"
        "mov byte ptr [rsi + rcx*8 + 32], 0\n"
        "mov qword ptr [rip + msg], rax\n"
        "mov rax, qword [rel msg]\n"
        "mov rax, fs:[0x28]\n"
        "mov es:[di], al\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
    EXPECT_NE(find(node, "MemoryOperand"), nullptr);
}

// --- Prefixes and String Instructions ---

TEST_P(AsmGrammarFixture, parses_x86_prefixes_and_strings) {
    const char *source =
        "lock cmpxchg [rdi], rsi\n"
        "rep stosb\n"
        "rep movsb\n"
        "repe cmpsb\n"
        "repne scasb\n"
        "notrack jmp rax\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
    EXPECT_NE(find(node, "InstructionPrefix"), nullptr);
}

// --- Control Flow and Calls ---

TEST_P(AsmGrammarFixture, parses_control_flow_and_calls) {
    const char *source =
        "jmp _start\n"
        "jmp short .L1\n"
        "call printf\n"
        "call near [rax]\n"
        "je .Lexit\n"
        "jne .Lloop\n"
        "ja .Labove\n"
        "jb .Lbelow\n"
        "jg .Lgreater\n"
        "jl .Lless\n"
        "ret\n"
        "ret 8\n"
        "retn\n"
        "retf\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
}

// --- SIMD and AVX Instructions ---

TEST_P(AsmGrammarFixture, parses_simd_avx_instructions) {
    const char *source =
        "movaps xmm0, xmm1\n"
        "movups xmm0, [rdi]\n"
        "addps xmm0, xmm1\n"
        "subps xmm0, xmm1\n"
        "mulps xmm0, xmm1\n"
        "divps xmm0, xmm1\n"
        "vmovaps ymm0, ymm1\n"
        "vaddps ymm0, ymm1, ymm2\n"
        "vsubps ymm0, ymm1, ymm2\n"
        "vzeroall\n"
        "vzeroupper\n"
        "pxor xmm0, xmm0\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
}

// --- ARM AArch64 Instructions ---

TEST_P(AsmGrammarFixture, parses_arm_aarch64_instructions) {
    const char *source =
        "mov x0, #1\n"
        "mov w0, #0\n"
        "ldr x1, =msg\n"
        "ldr x0, [x1]\n"
        "ldr x0, [x1, #8]\n"
        "ldr x0, [x1, #8]!\n"
        "ldr x0, [x1], #8\n"
        "str x0, [sp, #-16]!\n"
        "ldp x29, x30, [sp], #16\n"
        "stp x29, x30, [sp, #-16]!\n"
        "add x0, x1, x2\n"
        "sub x0, x1, #1\n"
        "cmp x0, #0\n"
        "csel x0, x1, x2, eq\n"
        "b .L1\n"
        "bl printf\n"
        "b.eq .L1\n"
        "b.ne .L2\n"
        "cbz x0, .Lexit\n"
        "cbnz w0, .Lloop\n"
        "svc #0\n"
        "brk #0\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
    EXPECT_NE(find(node, "MemoryOperand"), nullptr);
}

// --- ARM AArch32 and Shifted Operands ---

TEST_P(AsmGrammarFixture, parses_arm_aarch32_and_shifts) {
    const char *source =
        "add r0, r1, r2\n"
        "add r0, r1, r2, lsl #2\n"
        "sub r0, r1, r2, asr #1\n"
        "ldr r0, [r1, r2, lsl #2]\n"
        "push {r4, r5, lr}\n"
        "pop {r4, r5, pc}\n"
        "ldmfd sp!, {r4-r7, pc}\n"
        "stmfd sp!, {r4-r7, lr}\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
    EXPECT_NE(find(node, "ARMRegisterList"), nullptr);
}

// --- AT&T Syntax ---

TEST_P(AsmGrammarFixture, parses_att_syntax) {
    const char *source =
        "movl %eax, %ebx\n"
        "movq 8(%rbp), %rax\n"
        "movl (%rax,%rcx,4), %edx\n"
        "movl $42, %eax\n"
        "leaq (%rip), %rax\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
}

// --- Data Directives ---

TEST_P(AsmGrammarFixture, parses_data_directives) {
    const char *source =
        "msg db 'Hello', 0\n"
        "words dw 1, 2, 3\n"
        "nums dd 100, 200, 300\n"
        "quads dq 0x12345678, 0x9abcdef0\n"
        "buf resb 64\n"
        "space resd 16\n"
        ".byte 1, 2, 3\n"
        ".word 0x1000\n"
        ".long 100000\n"
        ".quad 200000\n"
        ".ascii \"Hello\\n\"\n"
        ".asciz \"World\"\n"
        ".string \"Test\"\n"
        ".zero 32\n"
        ".fill 10, 4, 0\n"
        "arr dd 100 dup(0)\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DataDefinition"), nullptr);
}

// --- Equates and Constants ---

TEST_P(AsmGrammarFixture, parses_equates_and_constants) {
    const char *source =
        "len equ $ - msg\n"
        "BUFFER_SIZE = 4096\n"
        ".equ MAX_ITEMS, 128\n"
        ".set TEMP_VAL, 1\n"
        "MASK equ (1 << 8) - 1\n"
        "COMPLEX equ ((10 + 20) * 2) / 4\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "EquateStatement"), nullptr);
}

// --- GAS CFI and Metadata Directives ---

TEST_P(AsmGrammarFixture, parses_gas_cfi_and_metadata_directives) {
    const char *source =
        ".cfi_startproc\n"
        ".cfi_def_cfa_offset 16\n"
        ".cfi_offset 6, -16\n"
        ".cfi_def_cfa_register 6\n"
        ".cfi_endproc\n"
        ".file \"test.s\"\n"
        ".ident \"GCC: (GNU) 12.2.0\"\n"
        ".type _start, @function\n"
        ".size _start, .- _start\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "DirectiveStatement"), nullptr);
}

// --- Macros and Procedures ---

TEST_P(AsmGrammarFixture, parses_macros_and_procedures) {
    const char *source =
        "%macro print_str 2\n"
        "    mov rax, 1\n"
        "    mov rdi, 1\n"
        "    mov rsi, %1\n"
        "    mov rdx, %2\n"
        "    syscall\n"
        "%endmacro\n"
        "\n"
        "my_func proc near\n"
        "    push rbp\n"
        "    mov rbp, rsp\n"
        "    pop rbp\n"
        "    ret\n"
        "my_func endp\n"
        "\n"
        ".macro my_gas_macro arg1, arg2\n"
        "    nop\n"
        ".endm\n"
        "\n"
        ".func my_gas_func\n"
        "    nop\n"
        "    ret\n"
        ".endfunc\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "MacroDefinition"), nullptr);
    EXPECT_NE(find(node, "ProcedureDefinition"), nullptr);
}

// --- Conditionals and Times ---

TEST_P(AsmGrammarFixture, parses_conditionals_and_times) {
    const char *source =
        "%ifdef DEBUG\n"
        "    mov rax, 1\n"
        "%else\n"
        "    mov rax, 0\n"
        "%endif\n"
        "\n"
        ".ifdef DEBUG\n"
        "    nop\n"
        ".else\n"
        "    ret\n"
        ".endif\n"
        "\n"
        "times 510-($-$$) db 0\n";
    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ConditionalBlock"), nullptr);
    EXPECT_NE(find(node, "TimesStatement"), nullptr);
}

// --- Error Recovery ---

TEST_P(AsmGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "bad_instruction ?? !! @@\n"
        "_recover_here:\n"
        "    mov rax, 42\n";
    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "InstructionStatement"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(AsmGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace
