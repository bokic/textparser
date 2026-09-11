package com.textparser;

import java.util.List;

public class TextParserTest {

    private static final String JSON_DEF = """
    {
      "name": "json",
      "version": 1.0,
      "otherTextInside": false,
      "startTokens": ["Object", "Array"],
      "tokens": {
        "Object": {
          "type": "StartStop",
          "startRegex": "\\\\{",
          "endRegex": "\\\\}",
          "otherTextInside": true,
          "nestedTokens": ["Key", "String", "KeyValueSeparator", "Value", "ValueSeparator"]
        },
        "Array": {
          "type": "StartStop",
          "startRegex": "\\\\[",
          "endRegex": "\\\\]",
          "otherTextInside": true,
          "nestedTokens": ["String", "Number", "Object", "Array", "Bool", "Null", "ValueSeparator"]
        },
        "Value": {
          "type": "GroupOneChildOnly",
          "nestedTokens": ["String", "Number", "Object", "Array", "Bool", "Null"]
        },
        "Key": {
          "type": "StartStop",
          "startRegex": "\\"",
          "endRegex": "\\"",
          "otherTextInside": true,
          "nestedTokens": ["StringChar"]
        },
        "String": {
          "type": "StartStop",
          "startRegex": "\\"",
          "endRegex": "\\"",
          "otherTextInside": true,
          "nestedTokens": ["StringChar"]
        },
        "StringChar": {
          "type": "SimpleToken",
          "startRegex": "[^\\"\\\\\\\\\\\\r\\\\n]+"
        },
        "Number": {
          "type": "SimpleToken",
          "startRegex": "-?[0-9]+(?:\\\\.[0-9]+)?"
        },
        "Bool": {
          "type": "SimpleToken",
          "startRegex": "true|false"
        },
        "Null": {
          "type": "SimpleToken",
          "startRegex": "null"
        },
        "KeyValueSeparator": {
          "type": "SimpleToken",
          "startRegex": ":"
        },
        "ValueSeparator": {
          "type": "SimpleToken",
          "startRegex": ","
        }
      }
    }
    """;

    private static final String CALC_DEF = """
    {
      "name": "calc",
      "startTokens": ["Expr"],
      "mergeSignIntoNumber": {
        "signTokens": ["Sign"],
        "numberTokens": ["Number", "Expr"],
        "operandTokens": ["Number", "Var"]
      },
      "tokens": {
        "Expr": {
          "type": "Group",
          "otherTextInside": true,
          "deleteIfOnlyOneChild": true,
          "nestedTokens": ["Sign", "Number", "Var", "Op"]
        },
        "Sign": {
          "type": "SimpleToken",
          "startRegex": "[-+]"
        },
        "Number": {
          "type": "SimpleToken",
          "startRegex": "[0-9]+"
        },
        "Var": {
          "type": "SimpleToken",
          "startRegex": "[a-zA-Z]+"
        },
        "Op": {
          "type": "SimpleToken",
          "startRegex": "[*/]"
        }
      }
    }
    """;

    private static final String DELIM_DEF = """
    {
      "name": "delim_test",
      "version": 1.0,
      "caseSensitivity": false,
      "defaultFileExtensions": ["dt"],
      "defaultTextEncoding": "utf-8",
      "otherTextInside": true,
      "startTokens": ["Styled", "Plain", "StyledNested", "PlainNested", "StyledBold"],
      "tokens": {
        "Styled": {"type": "StartStop", "startRegex": "\\\\[", "endRegex": "\\\\]", "otherTextInside": true, "delimiterTextColor": "0xff0000"},
        "Plain": {"type": "StartStop", "startRegex": "<", "endRegex": ">", "otherTextInside": true},
        "StyledNested": {"type": "StartStop", "startRegex": "\\\\{", "endRegex": "\\\\}", "otherTextInside": true, "nestedTokens": ["Word"], "delimiterTextColor": "0xff0000"},
        "PlainNested": {"type": "StartStop", "startRegex": "\\\\(", "endRegex": "\\\\)", "otherTextInside": true, "nestedTokens": ["Word"]},
        "StyledBold": {"type": "StartStop", "startRegex": "\\\\*\\\\*", "endRegex": "\\\\*\\\\*", "otherTextInside": true, "nestedTokens": ["StyledItalic"], "delimiterTextColor": "0xff0000"},
        "StyledItalic": {"type": "StartStop", "startRegex": "\\\\*", "endRegex": "\\\\*", "otherTextInside": true, "delimiterTextColor": "0x00ff00"},
        "Word": {"type": "SimpleToken", "startRegex": "[a-z]+"}
      }
    }
    """;

    private static final String RECOVER_DEF = """
    {
      "name": "recover_test",
      "version": 1.0,
      "caseSensitivity": false,
      "defaultFileExtensions": ["rt"],
      "defaultTextEncoding": "utf-8",
      "otherTextInside": true,
      "startTokens": ["Recover"],
      "tokens": {
        "Recover": {"type": "StartStop", "startRegex": "<", "endRegex": ">", "otherTextInside": true, "nestedTokens": ["Str"]},
        "Str": {"type": "StartStop", "startRegex": "\\\\\\"", "endRegex": "\\\\\\"", "otherTextInside": true}
      }
    }
    """;

    private static final String MULTILINE_DEF = """
    {
      "name": "multiline_test",
      "version": 1.0,
      "caseSensitivity": false,
      "defaultFileExtensions": ["mt"],
      "defaultTextEncoding": "utf-8",
      "otherTextInside": true,
      "startTokens": ["Outer"],
      "tokens": {
        "Outer": {"type": "StartStop", "startRegex": "\\\\{", "endRegex": "\\\\}", "multiLine": true, "otherTextInside": true, "nestedTokens": ["Wrap"]},
        "Wrap": {"type": "GroupOneChildOnly", "nestedTokens": ["Multi"]},
        "Multi": {"type": "StartStop", "startRegex": "\\\\[", "endRegex": "\\\\]", "multiLine": true, "otherTextInside": true, "nestedTokens": ["Digit"]},
        "Digit": {"type": "SimpleToken", "startRegex": "[0-9]+"}
      }
    }
    """;

    public static void main(String[] args) {
        int passed = 0;
        int failed = 0;

        try {
            testParseJsonObject();
            passed++;
            System.out.println("[PASS] testParseJsonObject");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testParseJsonObject: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testParseJsonArray();
            passed++;
            System.out.println("[PASS] testParseJsonArray");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testParseJsonArray: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testParseFormat();
            passed++;
            System.out.println("[PASS] testParseFormat");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testParseFormat: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testEmptyAndWhitespaceInput();
            passed++;
            System.out.println("[PASS] testEmptyAndWhitespaceInput");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testEmptyAndWhitespaceInput: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testInvalidDefinitionJson();
            passed++;
            System.out.println("[PASS] testInvalidDefinitionJson");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testInvalidDefinitionJson: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testSignMergeAndPostProcess();
            passed++;
            System.out.println("[PASS] testSignMergeAndPostProcess");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testSignMergeAndPostProcess: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testMissingStartRegexDoesNotMatchInput();
            passed++;
            System.out.println("[PASS] testMissingStartRegexDoesNotMatchInput");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testMissingStartRegexDoesNotMatchInput: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testUnmatchedRequiredDelimiterKeepsExistingTokenLengthBehavior();
            passed++;
            System.out.println("[PASS] testUnmatchedRequiredDelimiterKeepsExistingTokenLengthBehavior");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testUnmatchedRequiredDelimiterKeepsExistingTokenLengthBehavior: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testSequenceTokenType();
            passed++;
            System.out.println("[PASS] testSequenceTokenType");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testSequenceTokenType: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testTokenTypeEnum();
            passed++;
            System.out.println("[PASS] testTokenTypeEnum");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testTokenTypeEnum: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testToJson();
            passed++;
            System.out.println("[PASS] testToJson");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testToJson: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testDelimiterStylingNoNestedSingleSpan();
            passed++;
            System.out.println("[PASS] testDelimiterStylingNoNestedSingleSpan");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testDelimiterStylingNoNestedSingleSpan: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testDelimiterStylingSkipsLeadingWhitespace();
            passed++;
            System.out.println("[PASS] testDelimiterStylingSkipsLeadingWhitespace");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testDelimiterStylingSkipsLeadingWhitespace: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testDelimiterStylingWhitespaceOnly();
            passed++;
            System.out.println("[PASS] testDelimiterStylingWhitespaceOnly");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testDelimiterStylingWhitespaceOnly: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testNoDelimiterStylingPrunesPlainNestedSingleWord();
            passed++;
            System.out.println("[PASS] testNoDelimiterStylingPrunesPlainNestedSingleWord");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testNoDelimiterStylingPrunesPlainNestedSingleWord: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testNoDelimiterStylingKeepsLeadingWhitespace();
            passed++;
            System.out.println("[PASS] testNoDelimiterStylingKeepsLeadingWhitespace");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testNoDelimiterStylingKeepsLeadingWhitespace: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testStyledNestedSplitsUnprocessedOnWhitespace();
            passed++;
            System.out.println("[PASS] testStyledNestedSplitsUnprocessedOnWhitespace");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testStyledNestedSplitsUnprocessedOnWhitespace: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testEndDelimiterWinsOverNestedChild();
            passed++;
            System.out.println("[PASS] testEndDelimiterWinsOverNestedChild");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testEndDelimiterWinsOverNestedChild: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testTopLevelUnprocessedSplitsOnWhitespace();
            passed++;
            System.out.println("[PASS] testTopLevelUnprocessedSplitsOnWhitespace");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testTopLevelUnprocessedSplitsOnWhitespace: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testNoDelimiterStylingPrunesNoNestedWithInternalWhitespace();
            passed++;
            System.out.println("[PASS] testNoDelimiterStylingPrunesNoNestedWithInternalWhitespace");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testNoDelimiterStylingPrunesNoNestedWithInternalWhitespace: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testNestedFailureRecoversAsUnprocessed();
            passed++;
            System.out.println("[PASS] testNestedFailureRecoversAsUnprocessed");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testNestedFailureRecoversAsUnprocessed: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testNestedFailurePrunedWhenOnlyUnprocessed();
            passed++;
            System.out.println("[PASS] testNestedFailurePrunedWhenOnlyUnprocessed");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testNestedFailurePrunedWhenOnlyUnprocessed: " + t.getMessage());
            t.printStackTrace();
        }

        try {
            testMultiLineValidationRecovers();
            passed++;
            System.out.println("[PASS] testMultiLineValidationRecovers");
        } catch (Throwable t) {
            failed++;
            System.err.println("[FAIL] testMultiLineValidationRecovers: " + t.getMessage());
            t.printStackTrace();
        }

        System.out.println("\nTest Summary: " + passed + " PASSED, " + failed + " FAILED.");
        if (failed > 0) {
            System.exit(1);
        }
    }

    private static void assertTrue(boolean cond, String msg) {
        if (!cond) throw new AssertionError(msg);
    }

    private static void assertEquals(Object expected, Object actual, String msg) {
        if (expected == null && actual == null) return;
        if (expected != null && expected.equals(actual)) return;
        throw new AssertionError(msg + " | Expected: " + expected + ", Actual: " + actual);
    }

    public static void testParseJsonObject() {
        TextParser parser = new TextParser(JSON_DEF);
        String text = "{\"key\": \"value\", \"num\": 123, \"flag\": true}";
        List<TokenItem> tokens = parser.parse(text);

        assertEquals(1, tokens.size(), "Should have 1 top level token");
        assertEquals("Object", tokens.get(0).id, "Token should be Object");
        assertEquals(0, tokens.get(0).position, "Position should be 0");
        assertEquals(text.length(), tokens.get(0).length, "Length should match input");
        assertTrue(!tokens.get(0).children.isEmpty(), "Object should have children");
    }

    public static void testParseJsonArray() {
        TextParser parser = new TextParser(JSON_DEF);
        String text = "[1, 2, \"three\", false, null]";
        List<TokenItem> tokens = parser.parse(text);

        assertEquals(1, tokens.size(), "Should have 1 top level token");
        assertEquals("Array", tokens.get(0).id, "Token should be Array");
        assertEquals(text.length(), tokens.get(0).length, "Length should match input");
    }

    public static void testParseFormat() {
        TextParser parser = new TextParser(JSON_DEF);
        String text = "{\"a\": 1}";
        byte[] formatted = parser.parseFormat(text);

        assertEquals(text.length(), formatted.length, "Format length must match text length");
        boolean nonZeroFound = false;
        for (byte b : formatted) {
            if (b != 0) {
                nonZeroFound = true;
                break;
            }
        }
        assertTrue(nonZeroFound, "Format array should contain non-zero token indices");
    }

    public static void testEmptyAndWhitespaceInput() {
        TextParser parser = new TextParser(JSON_DEF);
        List<TokenItem> tokens = parser.parse("   \n\t  ");
        assertTrue(tokens.isEmpty(), "Whitespace input should yield empty token list");
    }

    public static void testInvalidDefinitionJson() {
        boolean exceptionThrown = false;
        try {
            new TextParser("{ invalid json ");
        } catch (Exception e) {
            exceptionThrown = true;
        }
        assertTrue(exceptionThrown, "Invalid JSON should throw exception");
    }

    public static void testSignMergeAndPostProcess() {
        TextParser parser = new TextParser(CALC_DEF);
        List<TokenItem> tokens = parser.parse("-123");

        assertEquals(1, tokens.size(), "Should have 1 top level token after merge & post-process");
        assertEquals("Number", tokens.get(0).id, "Token should be Number");
        assertEquals(0, tokens.get(0).position, "Position should be 0");
        assertEquals(4, tokens.get(0).length, "Length should be 4 (-123)");
    }

    public static void testMissingStartRegexDoesNotMatchInput() {
        String def = """
        {
          "startTokens": ["Number"],
          "tokens": {"Number": {"type": "SimpleToken"}}
        }
        """;
        TextParser parser = new TextParser(def);
        List<TokenItem> tokens = parser.parse("1");
        assertEquals(1, tokens.size(), "Unmatched remainder is emitted as one Unprocessed span");
        assertEquals(TokenItem.UNPROCESSED, tokens.get(0).id, "Token id");
        assertEquals(0, tokens.get(0).position, "Position");
        assertEquals(1, tokens.get(0).length, "Length");
    }

    public static void testUnmatchedRequiredDelimiterKeepsExistingTokenLengthBehavior() {
        TextParser parser = new TextParser(JSON_DEF);
        String text = "{\"key\": 1";
        List<TokenItem> tokens = parser.parse(text);
        assertEquals("Object", tokens.get(0).id, "Token should be Object");
        assertEquals(text.length(), tokens.get(0).length, "Unmatched delimiter should match to end of text");
    }

    public static void testSequenceTokenType() {
        String seqDef = """
        {
          "name": "sequence_test",
          "startTokens": ["TaggedType"],
          "tokens": {
            "TaggedType": {
              "type": "Sequence",
              "nestedTokens": ["TagSpecifier", "TypeName"]
            },
            "TagSpecifier": {
              "type": "SimpleToken",
              "startRegex": "struct\\\\b|union\\\\b|enum\\\\b"
            },
            "TypeName": {
              "type": "SimpleToken",
              "startRegex": "[a-zA-Z_][a-zA-Z0-9_]*"
            }
          }
        }
        """;
        TextParser parser = new TextParser(seqDef);
        String text = "struct Point";
        List<TokenItem> tokens = parser.parse(text);

        assertEquals(1, tokens.size(), "Should parse 1 sequence token");
        TokenItem item = tokens.get(0);
        assertEquals("TaggedType", item.id, "Top level token should be TaggedType");
        assertEquals(0, item.position, "Position should be 0");
        assertEquals(text.length(), item.length, "Length should match text");
        assertEquals(2, item.children.size(), "Sequence should have 2 children");
        assertEquals("TagSpecifier", item.children.get(0).id, "First child should be TagSpecifier");
        assertEquals("TypeName", item.children.get(1).id, "Second child should be TypeName");

        // Failure case: incomplete sequence is left as one Unprocessed span
        List<TokenItem> incomplete = parser.parse("struct 123");
        assertEquals(1, incomplete.size(), "Incomplete sequence should yield one Unprocessed span");
        assertEquals(TokenItem.UNPROCESSED, incomplete.get(0).id, "Token id");
        assertEquals(0, incomplete.get(0).position, "Position");
        assertEquals(10, incomplete.get(0).length, "Length");
    }

    public static void testTokenTypeEnum() {
        assertEquals(Definition.TokenType.Group, Definition.TokenType.fromString("Group"), "Group token type");
        assertEquals(Definition.TokenType.GroupAllChildrenInSameOrder, Definition.TokenType.fromString("GroupAllChildrenInSameOrder"), "GroupAllChildrenInSameOrder token type");
        assertEquals(Definition.TokenType.GroupOneChildOnly, Definition.TokenType.fromString("GroupOneChildOnly"), "GroupOneChildOnly token type");
        assertEquals(Definition.TokenType.SimpleToken, Definition.TokenType.fromString("SimpleToken"), "SimpleToken token type");
        assertEquals(Definition.TokenType.StartStop, Definition.TokenType.fromString("StartStop"), "StartStop token type");
        assertEquals(Definition.TokenType.StartOptStop, Definition.TokenType.fromString("StartOptStop"), "StartOptStop token type");
        assertEquals(Definition.TokenType.Sequence, Definition.TokenType.fromString("Sequence"), "Sequence token type");
        assertEquals(null, Definition.TokenType.fromString(null), "Null token type should return null");

        boolean threw = false;
        try {
            Definition.TokenType.fromString("NonExistentTokenType");
        } catch (IllegalArgumentException e) {
            threw = true;
        }
        assertTrue(threw, "Unknown token type string must throw IllegalArgumentException");
    }

    public static void testToJson() {
        TokenItem parent = new TokenItem("Parent", 0, 10);
        TokenItem child1 = new TokenItem("Child1", 0, 4);
        TokenItem child2 = new TokenItem("Child2", 5, 5);
        parent.children.add(child1);
        parent.children.add(child2);

        String json = parent.toJson();
        assertTrue(json.contains("\"id\": \"Parent\""), "JSON should contain parent id");
        assertTrue(json.contains("\"position\": 0"), "JSON should contain parent position");
        assertTrue(json.contains("\"length\": 10"), "JSON should contain parent length");
        assertTrue(json.contains("\"children\": ["), "JSON should contain children array");
        assertTrue(json.contains("\"id\": \"Child1\""), "JSON should contain child1 id");
        assertTrue(json.contains("\"id\": \"Child2\""), "JSON should contain child2 id");

        String compactJson = parent.toJson(false);
        assertTrue(!compactJson.contains("\n"), "Compact JSON should not contain newlines");

        String arrayJson = TokenItem.toJson(List.of(parent));
        assertTrue(arrayJson.trim().startsWith("[") && arrayJson.trim().endsWith("]"), "List toJson should produce JSON array");
    }

    private static void assertChild(TokenItem parent, int index, String id, int position, int length) {
        assertTrue(parent.children.size() > index,
            "Expected child index " + index + " but only " + parent.children.size() + " children present");
        TokenItem child = parent.children.get(index);
        assertEquals(id, child.id, "Child " + index + " id");
        assertEquals(position, child.position, "Child " + index + " position");
        assertEquals(length, child.length, "Child " + index + " length");
    }

    public static void testDelimiterStylingNoNestedSingleSpan() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("[a b]");

        assertEquals(1, tokens.size(), "Should have 1 top level token");
        TokenItem styled = tokens.get(0);
        assertEquals("Styled", styled.id, "Token id");
        assertEquals(0, styled.position, "Position");
        assertEquals(5, styled.length, "Length");
        assertEquals(3, styled.children.size(), "Styled token should expose start, content and end");
        assertChild(styled, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(styled, 1, TokenItem.UNPROCESSED, 1, 3);
        assertChild(styled, 2, TokenItem.END_DELIMITER, 4, 1);
    }

    public static void testDelimiterStylingSkipsLeadingWhitespace() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("[ a]");

        TokenItem styled = tokens.get(0);
        assertEquals(3, styled.children.size(), "Children count");
        assertChild(styled, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(styled, 1, TokenItem.UNPROCESSED, 2, 1);
        assertChild(styled, 2, TokenItem.END_DELIMITER, 3, 1);
    }

    public static void testDelimiterStylingWhitespaceOnly() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("[ ]");

        TokenItem styled = tokens.get(0);
        assertEquals(2, styled.children.size(), "Whitespace only content yields only delimiters");
        assertChild(styled, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(styled, 1, TokenItem.END_DELIMITER, 2, 1);
    }

    public static void testNoDelimiterStylingPrunesPlainNestedSingleWord() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("(1)");

        TokenItem plain = tokens.get(0);
        assertEquals("PlainNested", plain.id, "Token id");
        assertTrue(plain.children.isEmpty(), "Single unprocessed span without styling should be pruned");
    }

    public static void testNoDelimiterStylingPrunesNoNestedWithInternalWhitespace() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("<a b>");

        TokenItem plain = tokens.get(0);
        assertEquals("Plain", plain.id, "Token id");
        assertEquals(5, plain.length, "Length");
        assertTrue(plain.children.isEmpty(),
            "No-nested tokens fold internal whitespace into one span, so simple content is pruned");
    }

    public static void testNoDelimiterStylingKeepsLeadingWhitespace() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("< a b>");

        TokenItem plain = tokens.get(0);
        assertEquals(3, plain.children.size(), "Leading whitespace must prevent pruning");
        assertChild(plain, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(plain, 1, TokenItem.UNPROCESSED, 2, 3);
        assertChild(plain, 2, TokenItem.END_DELIMITER, 5, 1);
    }

    public static void testStyledNestedSplitsUnprocessedOnWhitespace() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("{1 2}");

        TokenItem styled = tokens.get(0);
        assertEquals(4, styled.children.size(), "Nested styled token splits content on whitespace");
        assertChild(styled, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(styled, 1, TokenItem.UNPROCESSED, 1, 1);
        assertChild(styled, 2, TokenItem.UNPROCESSED, 3, 1);
        assertChild(styled, 3, TokenItem.END_DELIMITER, 4, 1);
    }

    public static void testEndDelimiterWinsOverNestedChild() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("**A**");

        TokenItem bold = tokens.get(0);
        assertEquals("StyledBold", bold.id, "Token id");
        assertEquals(5, bold.length, "Length");
        assertEquals(3, bold.children.size(), "End delimiter must take priority over nested child");
        assertChild(bold, 0, TokenItem.START_DELIMITER, 0, 2);
        assertChild(bold, 1, TokenItem.UNPROCESSED, 2, 1);
        assertChild(bold, 2, TokenItem.END_DELIMITER, 3, 2);
    }

    public static void testTopLevelUnprocessedSplitsOnWhitespace() {
        TextParser parser = new TextParser(DELIM_DEF);
        List<TokenItem> tokens = parser.parse("  a  b   ");

        assertEquals(2, tokens.size(), "Top level unprocessed text is split on whitespace");
        assertEquals(TokenItem.UNPROCESSED, tokens.get(0).id, "First token id");
        assertEquals(2, tokens.get(0).position, "First token position");
        assertEquals(1, tokens.get(0).length, "First token length");
        assertEquals(TokenItem.UNPROCESSED, tokens.get(1).id, "Second token id");
        assertEquals(5, tokens.get(1).position, "Second token position");
        assertEquals(1, tokens.get(1).length, "Second token length");
    }

    public static void testNestedFailureRecoversAsUnprocessed() {
        TextParser parser = new TextParser(RECOVER_DEF);
        List<TokenItem> tokens = parser.parse("<\"ab\"c>");

        TokenItem recover = tokens.get(0);
        assertEquals("Recover", recover.id, "Token id");
        assertEquals(7, recover.length, "Length");
        assertEquals(4, recover.children.size(), "Failed nested attempt must recover as Unprocessed");
        assertChild(recover, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(recover, 1, "Str", 1, 4);
        assertChild(recover, 2, TokenItem.UNPROCESSED, 5, 1);
        assertChild(recover, 3, TokenItem.END_DELIMITER, 6, 1);
    }

    public static void testNestedFailurePrunedWhenOnlyUnprocessed() {
        TextParser parser = new TextParser(RECOVER_DEF);
        List<TokenItem> tokens = parser.parse("<\"abc>");

        TokenItem recover = tokens.get(0);
        assertEquals("Recover", recover.id, "Token id");
        assertEquals(6, recover.length, "Length");
        assertTrue(recover.children.isEmpty(),
            "Recovered single Unprocessed span without styling should be pruned");
    }

    public static void testMultiLineValidationRecovers() {
        TextParser parser = new TextParser(MULTILINE_DEF);
        List<TokenItem> tokens = parser.parse("{[\n1\n]}");

        TokenItem outer = tokens.get(0);
        assertEquals("Outer", outer.id, "Token id");
        assertEquals(7, outer.length, "Length");
        assertEquals(5, outer.children.size(), "Multi-line group violation must recover as Unprocessed");
        assertChild(outer, 0, TokenItem.START_DELIMITER, 0, 1);
        assertChild(outer, 1, TokenItem.UNPROCESSED, 1, 1);
        assertChild(outer, 2, TokenItem.UNPROCESSED, 3, 1);
        assertChild(outer, 3, TokenItem.UNPROCESSED, 5, 1);
        assertChild(outer, 4, TokenItem.END_DELIMITER, 6, 1);
    }
}
