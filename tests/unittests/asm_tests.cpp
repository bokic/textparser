#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>
#include <asm.h>
#include <set>
#include <string>
#include <vector>

#include <asm_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_ASM, basic_asm_program) {
    auto tokens = TextParser(R"(; x86-64 assembly example
section .data
    msg db 'Hello World', 0
    len equ $ - msg

section .text
    global _start

_start:
    ; write syscall
    mov rax, 1
    mov rdi, 1
    mov rsi, msg
    mov rdx, len
    syscall

    ; exit syscall
    mov rax, 60
    xor rdi, rdi
    syscall
)", &asm_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("SectionDirective"));
    EXPECT_TRUE(found.contains("DataDirective"));
    EXPECT_TRUE(found.contains("EquDirective"));
    EXPECT_TRUE(found.contains("SymbolDirective"));
    EXPECT_TRUE(found.contains("InstructionMnemonic"));
    EXPECT_TRUE(found.contains("Register"));
    EXPECT_TRUE(found.contains("DecNumber"));
    EXPECT_TRUE(found.contains("Identifier"));
    EXPECT_TRUE(found.contains("Minus"));
    EXPECT_TRUE(found.contains("Dollar"));
    EXPECT_TRUE(found.contains("CharLiteral"));
}

struct ASMValidationFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/asm_definition.json", &json_definition),
                      TEXTPARSER_JSON_NO_ERROR);
            ASSERT_NE(json_definition, nullptr);
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

    std::vector<std::string> validate(const char *asm_content) {
        std::vector<std::string> messages;
        textparser_t handle = nullptr;
        int res = textparser_openmem(asm_content, strlen(asm_content), TEXTPARSER_ENCODING_UTF_8, &handle);
        if (res != 0) return messages;

        res = textparser_parse(handle, get_definition());
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        res = textparser_asm_register_validators(handle);
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        textparser_match_result match = {};
        res = textparser_execute_language_grammar(handle, get_definition(), &match);
        if (res != 0) {
            textparser_close(handle);
            return messages;
        }

        textparser_validation *val = textparser_validate_asm(handle);
        if (val) {
            for (int i = 0; i < val->len; i++) {
                if (val->items[i]->text) {
                    messages.push_back(val->items[i]->text);
                }
            }
            textparser_validation_clear(val);
        }
        textparser_close(handle);
        return messages;
    }

    bool has_error_code(const std::vector<std::string> &errors, const std::string &code) {
        for (const auto &err : errors) {
            if (err.find(code) != std::string::npos) return true;
        }
        return false;
    }
};

TEST_P(ASMValidationFixture, valid_asm_code) {
    const char *code = R"(
section .text
global _start

_start:
    xor eax, eax
    nop
    ret
)";
    auto errors = validate(code);
    EXPECT_TRUE(errors.empty());
}

TEST_P(ASMValidationFixture, duplicate_label_error) {
    const char *code = R"(
my_func:
    mov eax, 1
my_func:
    mov eax, 2
)";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2001"));
}

TEST_P(ASMValidationFixture, allowed_local_and_numeric_labels) {
    const char *code = R"(
.loop:
    nop
.loop:
    nop
1:
    nop
1:
    ret
)";
    auto errors = validate(code);
    EXPECT_FALSE(has_error_code(errors, "ASM2001"));
}

TEST_P(ASMValidationFixture, nullary_instruction_with_operands) {
    const char *code = "nop eax\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2002"));
}

TEST_P(ASMValidationFixture, nullary_syscall_with_operands) {
    const char *code = "syscall 1\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2002"));
}

TEST_P(ASMValidationFixture, nullary_cpuid_with_operands) {
    const char *code = "cpuid rax\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2002"));
}

TEST_P(ASMValidationFixture, unary_instruction_missing_operand) {
    const char *code = "push\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2002"));
}

TEST_P(ASMValidationFixture, binary_instruction_missing_operand) {
    const char *code = "mov eax\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2002"));
}

TEST_P(ASMValidationFixture, ambiguous_memory_operand_size_warning) {
    const char *code = "mov [rax], 1\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2003"));
}

TEST_P(ASMValidationFixture, specified_memory_operand_size_valid) {
    const char *code = "mov dword ptr [rax], 1\n";
    auto errors = validate(code);
    EXPECT_FALSE(has_error_code(errors, "ASM2003"));
}

TEST_P(ASMValidationFixture, register_source_memory_valid) {
    const char *code = "mov [rax], ebx\n";
    auto errors = validate(code);
    EXPECT_FALSE(has_error_code(errors, "ASM2003"));
}

TEST_P(ASMValidationFixture, duplicate_lock_prefix_error) {
    const char *code = "lock lock add dword ptr [rax], 1\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2004"));
}

TEST_P(ASMValidationFixture, invalid_lock_prefix_on_nop) {
    const char *code = "lock nop\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2004"));
}

TEST_P(ASMValidationFixture, valid_lock_prefix_on_add) {
    const char *code = "lock add dword ptr [rax], 1\n";
    auto errors = validate(code);
    EXPECT_FALSE(has_error_code(errors, "ASM2004"));
}

TEST_P(ASMValidationFixture, register_size_mismatch_error_32_16) {
    const char *code = "mov eax, bx\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2005"));
}

TEST_P(ASMValidationFixture, register_size_mismatch_error_64_32) {
    const char *code = "add rax, edx\n";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2005"));
}

TEST_P(ASMValidationFixture, matching_register_sizes_valid) {
    const char *code = R"(
mov eax, ebx
add rax, rdx
xor cl, dl
)";
    auto errors = validate(code);
    EXPECT_FALSE(has_error_code(errors, "ASM2005"));
}

TEST_P(ASMValidationFixture, duplicate_procedure_error) {
    const char *code = R"(
my_proc PROC
    ret
my_proc ENDP
my_proc PROC
    ret
my_proc ENDP
)";
    auto errors = validate(code);
    EXPECT_TRUE(has_error_code(errors, "ASM2006"));
}

INSTANTIATE_TEST_SUITE_P(
    Definitions,
    ASMValidationFixture,
    testing::Values(false, true),
    [](const testing::TestParamInfo<ASMValidationFixture::ParamType> &info) {
        return info.param ? "JSON" : "Static";
    });
