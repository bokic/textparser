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
}
