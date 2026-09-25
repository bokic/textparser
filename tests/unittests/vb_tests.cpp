#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <vb_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_VB, basic_vb_program) {
    auto tokens = TextParser(R"(
' Visual Basic class example
Public Class Greeter
    Private _name As String

    Public Sub New(name As String)
        _name = name
    End Sub

    Public Function Greet() As String
        Dim result As String
        result = "Hello, " & _name & "!"
        Return result
    End Function

    Public ReadOnly Property Name As String
        Get
            Return _name
        End Get
    End Property
End Class

Module Program
    Sub Main()
        Dim g As New Greeter("World")
        Console.WriteLine(g.Greet())
    End Sub
End Module
)", &vb_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("KwPublic"));
    EXPECT_TRUE(found.contains("KwPrivate"));
    EXPECT_TRUE(found.contains("KwClass"));
    EXPECT_TRUE(found.contains("KwSub"));
    EXPECT_TRUE(found.contains("KwFunction"));
    EXPECT_TRUE(found.contains("KwProperty"));
    EXPECT_TRUE(found.contains("KwModule"));
    EXPECT_TRUE(found.contains("KwDim"));
    EXPECT_TRUE(found.contains("KwReturn"));
    EXPECT_TRUE(found.contains("KwNew"));
    EXPECT_TRUE(found.contains("KwReadOnly"));
    EXPECT_TRUE(found.contains("KwGet"));
    EXPECT_TRUE(found.contains("KwEnd"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Ampersand"));
    EXPECT_TRUE(found.contains("Ident"));
}
