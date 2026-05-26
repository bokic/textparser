#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
#include <set>
#include <string>

#include <ada_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Ada, basic_ada_program) {
    auto tokens = TextParser(R"(
-- Ada package specification
package Math_Utils is

   type Vector is record
      X, Y, Z : Float;
   end record;

   function Dot_Product (A, B : Vector) return Float;
   function Length (V : Vector) return Float;

   Pi : constant Float := 3.14159;

private

   function Internal_Check (V : Vector) return Boolean;

end Math_Utils;

package body Math_Utils is

   function Dot_Product (A, B : Vector) return Float is
   begin
      return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
   end Dot_Product;

   function Length (V : Vector) return Float is
   begin
      return Sqrt (Dot_Product (V, V));
   end Length;

   function Internal_Check (V : Vector) return Boolean is
   begin
      return True;
   end Internal_Check;

begin
   null;
end Math_Utils;
)", &ada_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
}
