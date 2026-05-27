#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <fortran_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Fortran, basic_fortran_program) {
    auto tokens = TextParser(R"(
! Fortran module with derived type
module math_utils
  implicit none
  private

  type, public :: vector
    real :: x, y, z
  end type vector

  public :: dot_product, cross_product

contains

  function dot_product(a, b) result(res)
    type(vector), intent(in) :: a, b
    real :: res
    res = a%x * b%x + a%y * b%y + a%z * b%z
  end function dot_product

  subroutine print_vector(v)
    type(vector), intent(in) :: v
    print *, 'Vector: ', v%x, v%y, v%z
  end subroutine print_vector

end module math_utils

program test_prog
  use math_utils
  implicit none

  type(vector) :: v1, v2
  real :: d
  logical :: ok = .true.

  v1 = vector(1.0, 2.0, 3.0)
  v2 = vector(4.0d0, 5.0d0, 6.0d0)

  d = dot_product(v1, v2)
  if (d > 10.0) then
    print *, "Large dot product"
    print *, 'It''s single quoted'
    print *, "He said, ""Hello!"""
  end if

  call print_vector(v1)
end program test_prog
)", &fortran_definition);

    std::cout << "DETAILED TOKENS:\n";
    for (size_t i = 0; i < tokens.count; ++i) {
        auto item = tokens[i];
        std::cout << "Token[" << i << "]: type=" << (item.type ? item.type : "null")
                  << ", val='" << item.value << "', children=" << item.children << "\n";
        for (size_t j = 0; j < item.children; ++j) {
            auto child = item[j];
            std::cout << "  Child[" << j << "]: type=" << (child.type ? child.type : "null")
                      << ", val='" << child.value << "', children=" << child.children << "\n";
        }
    }

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
    EXPECT_TRUE(found.contains("SingleString"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("EscapedApostrophe"));
    EXPECT_TRUE(found.contains("EscapedDoubleQuote"));
}
