#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <fortran_definition.json.h>

#include <cstring>
#include <functional>
#include <string>
#include <iostream>

namespace {

struct FortranGrammarFixture : testing::TestWithParam<bool> {
    textparser_language_definition *json_definition = nullptr;

    void SetUp() override {
        if (GetParam()) {
            ASSERT_EQ(textparser_json_load_language_definition_from_json_file(
                          "definitions/fortran_definition.json", &json_definition),
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
        return GetParam() ? json_definition : &fortran_definition;
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
        if (textparser_get_diagnostic_count(parser.get()) != 0) {
            size_t diag_count = textparser_get_diagnostic_count(parser.get());
            std::cout << "Diagnostic count: " << diag_count << std::endl;
            for (size_t i = 0; i < diag_count; ++i) {
                textparser_diagnostic d = {};
                if (textparser_get_diagnostic(parser.get(), i, &d) == 0) {
                    std::cout << "Diag " << i << ": [" << (d.code ? d.code : "") << "] " << (d.message ? d.message : "") << " at pos " << d.start_pos << std::endl;
                }
            }
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

INSTANTIATE_TEST_SUITE_P(DefinitionSources, FortranGrammarFixture, testing::Bool(),
                         [](const testing::TestParamInfo<bool> &info) {
                             return info.param ? "JSON" : "Static";
                         });

// --- Basic Programs, Modules and Submodules ---

TEST_P(FortranGrammarFixture, parses_basic_program_and_modules) {
    const char *source =
        "! Fortran module and program\n"
        "module math_mod\n"
        "  implicit none\n"
        "  private\n"
        "  public :: add, multiply\n"
        "contains\n"
        "  function add(a, b) result(c)\n"
        "    real, intent(in) :: a, b\n"
        "    real :: c\n"
        "    c = a + b\n"
        "  end function add\n"
        "\n"
        "  subroutine multiply(a, b, res)\n"
        "    real, intent(in) :: a, b\n"
        "    real, intent(out) :: res\n"
        "    res = a * b\n"
        "  end subroutine multiply\n"
        "end module math_mod\n"
        "\n"
        "program main\n"
        "  use math_mod\n"
        "  implicit none\n"
        "  real :: x, y, z\n"
        "  x = 3.0\n"
        "  y = 4.0\n"
        "  z = add(x, y)\n"
        "  call multiply(x, y, z)\n"
        "  print *, 'Result: ', z\n"
        "end program main\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "ModuleDeclaration"), nullptr);
    EXPECT_NE(find(node, "Program"), nullptr);
    EXPECT_NE(find(node, "FunctionDeclaration"), nullptr);
    EXPECT_NE(find(node, "SubroutineDeclaration"), nullptr);
    EXPECT_NE(find(node, "UseStatement"), nullptr);
}

// --- Submodules ---

TEST_P(FortranGrammarFixture, parses_submodule) {
    const char *source =
        "submodule (math_mod) math_impl\n"
        "  implicit none\n"
        "contains\n"
        "  module procedure add\n"
        "  end procedure add\n"
        "end submodule math_impl\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "SubmoduleDeclaration"), nullptr);
}

// --- Type Declarations and Specifications ---

TEST_P(FortranGrammarFixture, parses_type_declarations_and_specs) {
    const char *source =
        "program types_demo\n"
        "  implicit none\n"
        "  integer, parameter :: dp = kind(1.0d0)\n"
        "  real(dp), dimension(10, 20), allocatable :: matrix\n"
        "  character(len=32) :: name = 'Fortran 2018'\n"
        "  complex(dp) :: cmp = (1.0_dp, -2.0_dp)\n"
        "  logical :: flag = .true.\n"
        "  integer :: a = 1, b = 2, c\n"
        "  integer, pointer :: ptr => null()\n"
        "  integer, target :: tgt = 42\n"
        "  save :: flag\n"
        "  ptr => tgt\n"
        "end program types_demo\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "VariableDeclaration"), nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Derived Types and OOP ---

TEST_P(FortranGrammarFixture, parses_derived_types_and_classes) {
    const char *source =
        "module shapes\n"
        "  implicit none\n"
        "  type, abstract :: shape\n"
        "    real :: x, y\n"
        "  contains\n"
        "    procedure(area_interface), deferred, pass :: area\n"
        "  end type shape\n"
        "\n"
        "  type, extends(shape) :: circle\n"
        "    real :: radius\n"
        "  contains\n"
        "    procedure, pass :: area => circle_area\n"
        "  end type circle\n"
        "\n"
        "  abstract interface\n"
        "    function area_interface(this) result(res)\n"
        "      import :: shape\n"
        "      class(shape), intent(in) :: this\n"
        "      real :: res\n"
        "    end function area_interface\n"
        "  end interface\n"
        "contains\n"
        "  function circle_area(this) result(res)\n"
        "    class(circle), intent(in) :: this\n"
        "    real :: res\n"
        "    res = 3.14159265 * this%radius ** 2\n"
        "  end function circle_area\n"
        "end module shapes\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "TypeDefinition"), nullptr);
    EXPECT_NE(find(node, "InterfaceDeclaration"), nullptr);
}

// --- Control Flow: If, Do, Select Case, Select Type ---

TEST_P(FortranGrammarFixture, parses_control_flow) {
    const char *source =
        "program control_flow\n"
        "  implicit none\n"
        "  integer :: i, val = 5\n"
        "  real :: x = 1.0\n"
        "\n"
        "  ! Block IF\n"
        "  if (val > 10) then\n"
        "    val = 10\n"
        "  else if (val < 0) then\n"
        "    val = 0\n"
        "  else\n"
        "    val = 5\n"
        "  end if\n"
        "\n"
        "  ! Logical IF\n"
        "  if (val == 5) x = 2.0\n"
        "\n"
        "  ! Counted DO\n"
        "  loop1: do i = 1, 10, 2\n"
        "    if (i == 5) cycle loop1\n"
        "    if (i == 9) exit loop1\n"
        "  end do loop1\n"
        "\n"
        "  ! DO WHILE\n"
        "  do while (x < 100.0)\n"
        "    x = x * 2.0\n"
        "  end do\n"
        "\n"
        "  ! DO CONCURRENT\n"
        "  do concurrent (i = 1:10)\n"
        "    x = x + i\n"
        "  end do\n"
        "\n"
        "  ! SELECT CASE\n"
        "  select case (val)\n"
        "  case (1:3)\n"
        "    print *, 'low'\n"
        "  case (4, 5, 6)\n"
        "    print *, 'mid'\n"
        "  case default\n"
        "    print *, 'other'\n"
        "  end select\n"
        "end program control_flow\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "IfStatement"), nullptr);
    EXPECT_NE(find(node, "DoStatement"), nullptr);
    EXPECT_NE(find(node, "SwitchStatement"), nullptr);
}

// --- Where, Forall, Block, Associate ---

TEST_P(FortranGrammarFixture, parses_where_forall_block_associate) {
    const char *source =
        "program modern_constructs\n"
        "  implicit none\n"
        "  integer :: a(5), b(5), i\n"
        "  a = [1, -2, 3, -4, 5]\n"
        "\n"
        "  where (a < 0)\n"
        "    b = -a\n"
        "  elsewhere\n"
        "    b = a\n"
        "  end where\n"
        "\n"
        "  forall (i = 1:5, b(i) > 2)\n"
        "    a(i) = b(i) * 2\n"
        "  end forall\n"
        "\n"
        "  block\n"
        "    integer :: temp\n"
        "    temp = a(1)\n"
        "    a(1) = a(2)\n"
        "    a(2) = temp\n"
        "  end block\n"
        "\n"
        "  associate (first => a(1), second => a(2))\n"
        "    first = second + 1\n"
        "  end associate\n"
        "end program modern_constructs\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
}

// --- I/O and Memory Allocation ---

TEST_P(FortranGrammarFixture, parses_io_and_allocations) {
    const char *source =
        "program io_demo\n"
        "  implicit none\n"
        "  integer, allocatable :: arr(:)\n"
        "  integer :: stat_code, n = 10\n"
        "\n"
        "  allocate (arr(n), stat=stat_code)\n"
        "  if (allocated(arr)) then\n"
        "    arr = 0\n"
        "    print *, 'Array allocated successfully'\n"
        "    write (*, '(A, I5)') 'Size = ', size(arr)\n"
        "  end if\n"
        "  deallocate (arr, stat=stat_code)\n"
        "end program io_demo\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AllocateStatement"), nullptr);
    EXPECT_NE(find(node, "DeallocateStatement"), nullptr);
    EXPECT_NE(find(node, "PrintStatement"), nullptr);
    EXPECT_NE(find(node, "WriteStatement"), nullptr);
}

// --- Pratt Operator Precedence and Expressions ---

TEST_P(FortranGrammarFixture, parses_pratt_operator_precedence) {
    const char *source =
        "program expr_test\n"
        "  implicit none\n"
        "  real :: a, b, c\n"
        "  logical :: ok\n"
        "  character(len=20) :: str\n"
        "  a = 1.0 + 2.0 * 3.0 ** 2.0\n"
        "  b = -2.0 ** 2.0\n"
        "  str = 'Hello ' // 'World'\n"
        "  ok = a > 10.0 .and. b < 0.0 .or. .not. (a == 5.0)\n"
        "  ok = ok .eqv. .true.\n"
        "end program expr_test\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
    EXPECT_NE(find(node, "AssignmentStatement"), nullptr);
}

// --- Array Constructors, Slices and BOZ Literals ---

TEST_P(FortranGrammarFixture, parses_arrays_and_boz) {
    const char *source =
        "program arrays_and_boz\n"
        "  implicit none\n"
        "  integer :: arr(5), v(10), mask\n"
        "  arr = [1, 2, 3, 4, 5]\n"
        "  v = (/ (i * 2, i = 1, 10) /)\n"
        "  v(1:5) = arr(:)\n"
        "  v(2:10:2) = 0\n"
        "  mask = z'FF00'\n"
        "end program arrays_and_boz\n";

    auto *node = parse_source(source);
    ASSERT_NE(node, nullptr);
}

// --- Error Recovery ---

TEST_P(FortranGrammarFixture, recovers_from_syntax_errors) {
    const char *source =
        "program recovery_test\n"
        "  implicit none\n"
        "  print *, 'First'\n"
        "  invalid statement ??? @@@ %%% ;\n"
        "  print *, 'Second'\n"
        "end program recovery_test\n";

    auto *node = parse_source(source, TEXTPARSER_MATCH_OK, true);
    ASSERT_NE(node, nullptr);
    ASSERT_GT(textparser_get_diagnostic_count(parser.get()), 0u);
    EXPECT_NE(find(node, "Program"), nullptr);
    EXPECT_NE(find(node, "PrintStatement"), nullptr);
}

// --- Rejects Malformed ---

TEST_P(FortranGrammarFixture, rejects_invalid_syntax) {
    const char *source = "[[[@@@???";
    parse_source(source, TEXTPARSER_MATCH_NO);
}

} // namespace
