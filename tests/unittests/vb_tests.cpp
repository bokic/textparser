#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.h>
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

    EXPECT_TRUE(found.contains("LineComment"));
    EXPECT_TRUE(found.contains("Keyword"));
    EXPECT_TRUE(found.contains("DataType"));
    EXPECT_TRUE(found.contains("DoubleString"));
    EXPECT_TRUE(found.contains("Variable"));
    EXPECT_TRUE(found.contains("Operator"));
}
