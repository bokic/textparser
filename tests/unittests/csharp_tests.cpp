#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>

#include <csharp_definition.json.h>
#include <set>

TEST(parse_CSharp, basic_csharp_program) {
    std::string source = R"(
// C# HelloWorld program
using System;
using System.Threading.Tasks;

/* A multi-line block comment
   to test BlockComment token */
namespace HelloNamespace {
    public class HelloWorld {
        public async Task<string> GetMessageAsync(int delayMs) {
            await Task.Delay(delayMs);
            char singleChar = '\n'; // SingleString with StringEscape
            char basicChar = 'a'; // SingleString
            string name = "World\n\t\"User\"";
            string defaultName = null;
            string finalName = name ?? defaultName;
            bool isActive = true;
            double score = 42.5;
            int hexNum = 0x2A; // hex number
            int binNum = 0b1010; // bin number
            int largeNum = 1_000_000; // digit separator
            Func<int, int> doubleVal = x => x * 2; // lambda operator =>
            global::System.Console.WriteLine("Done"); // namespace alias qualifier ::
            return $"Hello {finalName}!";
        }
    }
}
)";
    auto tokens = TextParser(source.c_str(), &csharp_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        if (tokens[i].type) {
            found.insert(tokens[i].type);
        }
    }

    EXPECT_TRUE(found.contains("UsingKeyword"));
    EXPECT_TRUE(found.contains("NamespaceKeyword"));
    EXPECT_TRUE(found.contains("PublicKeyword"));
    EXPECT_TRUE(found.contains("ClassKeyword"));
    EXPECT_TRUE(found.contains("AsyncKeyword"));
    EXPECT_TRUE(found.contains("AwaitKeyword"));
    EXPECT_TRUE(found.contains("ReturnKeyword"));
    EXPECT_TRUE(found.contains("NullKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("CharacterLiteral"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("NullCoalescing"));
    EXPECT_TRUE(found.contains("Arrow"));
    EXPECT_TRUE(found.contains("ScopeResolution"));
    EXPECT_TRUE(found.contains("Identifier"));
}
