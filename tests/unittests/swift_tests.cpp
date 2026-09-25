#include "tokenparser.hpp"

#include <gtest/gtest.h>
#include <textparser.hpp>
#include <set>
#include <string>

#include <swift_definition.json.h>

static void scan_tokens(const TokenParserItem &item, std::set<std::string> &found) {
    if (item.type) {
        found.insert(item.type);
    }
    for (size_t i = 0; i < item.children; ++i) {
        scan_tokens(item[i], found);
    }
}

TEST(parse_Swift, basic_swift_program) {
    auto tokens = TextParser(R"(
// Swift protocol and struct
protocol Greetable {
    var name: String { get }
    func greet() -> String
}

/* Generic struct with extension */
struct Greeter<T: Greetable> {
    private let subject: T

    init(subject: T) {
        self.subject = subject
    }

    func greet() -> String {
        return "Hello, \(subject.name)!"
    }
}

extension Greeter {
    mutating func reset() {
        // no-op
    }
}

let greeter = Greeter(subject: Person(name: "World"))
print(greeter.greet())

let isDone: Bool = true
let count = 42

let escapedStr = "Hello, \n World!"
let multilineStr = """
This is a multiline
string with \n escape and \(greeter.greet()) interpolation.
"""
)", &swift_definition);

    std::set<std::string> found;
    for (size_t i = 0; i < tokens.count; ++i) {
        scan_tokens(tokens[i], found);
    }

    EXPECT_TRUE(found.contains("ProtocolKeyword"));
    EXPECT_TRUE(found.contains("StructKeyword"));
    EXPECT_TRUE(found.contains("VarKeyword"));
    EXPECT_TRUE(found.contains("FuncKeyword"));
    EXPECT_TRUE(found.contains("InitKeyword"));
    EXPECT_TRUE(found.contains("LetKeyword"));
    EXPECT_TRUE(found.contains("ReturnKeyword"));
    EXPECT_TRUE(found.contains("ExtensionKeyword"));
    EXPECT_TRUE(found.contains("MutatingKeyword"));
    EXPECT_TRUE(found.contains("Boolean"));
    EXPECT_TRUE(found.contains("Number"));
    EXPECT_TRUE(found.contains("StringLiteral"));
    EXPECT_TRUE(found.contains("MultiLineStringLiteral"));
}

