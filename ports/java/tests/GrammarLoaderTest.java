package com.textparser;

import com.textparser.grammar.*;

import java.io.File;
import java.util.List;

public class GrammarLoaderTest {

    private static void assertTrue(boolean condition, String message) {
        if (!condition) throw new AssertionError("Assertion failed: " + message);
    }

    private static void assertEquals(Object expected, Object actual, String message) {
        if (expected == null && actual == null) return;
        if (expected != null && expected.equals(actual)) return;
        throw new AssertionError("Assertion failed: " + message + " (expected: " + expected + ", got: " + actual + ")");
    }

    private static String wrapGrammar(String grammarJson) {
        return """
        {
          "name": "test_grammar", "version": 2, "caseSensitivity": true,
          "defaultFileExtensions": ["txt"], "defaultTextEncoding": "utf-8",
          "startTokens": ["A", "B", "C"], "otherTextInside": true,
          "tokens": {
            "A": {"type": "SimpleToken", "startRegex": "a"},
            "B": {"type": "SimpleToken", "startRegex": "b"},
            "C": {"type": "SimpleToken", "startRegex": "c"}
          },
          "grammar": %s
        }
        """.formatted(grammarJson);
    }

    public static void testLoadsFlattenResolvesAndValidates() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "sequence": [
                {"token": "A"},
                {"repeat": {"ref": "BToken"}},
                {"optional": {"token": "C"}}
              ]
            },
            "BToken": {"token": "B"}
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(wrapGrammar(grammar));
        assertTrue(def != null, "Grammar definition loaded");
        assertEquals(0, def.startProduction, "Start production ID");
        Production root = def.getProduction(0);
        assertEquals("Root", root.name, "Root production name");
        assertEquals(ProductionKind.SEQUENCE, root.kind, "Root production kind");
        assertEquals(3, root.children.length, "Root sequence child count");

        Production bToken = def.getProductionByName("BToken");
        assertTrue(bToken != null, "BToken production exists");
        assertEquals(ProductionKind.TOKEN, bToken.kind, "BToken is TOKEN kind");
    }

    public static void testOneOrMoreDesugaring() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "oneOrMore": {"token": "A"},
              "astKind": "OneOrMoreA"
            }
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(wrapGrammar(grammar));
        Production root = def.getProduction(0);
        assertEquals(ProductionKind.SEQUENCE, root.kind, "oneOrMore desugars into SEQUENCE");
        assertEquals(2, root.children.length, "SEQUENCE has 2 children: item and repeat");
        assertEquals("OneOrMoreA", root.astKind, "astKind preserved on root");

        Production child1 = def.getProduction(root.children[0]);
        assertEquals(ProductionKind.TOKEN, child1.kind, "First child is item TOKEN");

        Production child2 = def.getProduction(root.children[1]);
        assertEquals(ProductionKind.REPEAT, child2.kind, "Second child is REPEAT");
        assertEquals(1, child2.children.length, "REPEAT has 1 child");

        Production repeatItem = def.getProduction(child2.children[0]);
        assertEquals(ProductionKind.TOKEN, repeatItem.kind, "REPEAT item is TOKEN");
    }

    public static void testRejectDirectLeftRecursion() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "choice": [
                {"ref": "Root"},
                {"token": "A"}
              ]
            }
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Direct left recursion must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_LEFT_RECURSION, caught.getErrorCode(), "Error code must be GRAMMAR_LEFT_RECURSION");
    }

    public static void testRejectIndirectLeftRecursion() {
        String grammar = """
        {
          "start": "AProd",
          "productions": {
            "AProd": {"ref": "BProd"},
            "BProd": {"sequence": [{"ref": "CProd"}, {"token": "B"}]},
            "CProd": {"ref": "AProd"}
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Indirect left recursion must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_LEFT_RECURSION, caught.getErrorCode(), "Error code must be GRAMMAR_LEFT_RECURSION");
    }

    public static void testAcceptRightRecursion() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "choice": [
                {"token": "A"},
                {"sequence": [{"token": "A"}, {"ref": "Root"}]}
              ]
            }
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(wrapGrammar(grammar));
        assertTrue(def != null, "Right recursion accepted");
    }

    public static void testRejectNullableRepeat() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "repeat": {"optional": {"token": "A"}}
            }
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Nullable repeat must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_NULLABLE_REPEAT, caught.getErrorCode(), "Error code must be GRAMMAR_NULLABLE_REPEAT");
    }

    public static void testRejectNullablePrattPostfix() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "pratt": {
                "primary": {"token": "A"},
                "postfix": {"optional": {"token": "B"}}
              }
            }
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Nullable pratt postfix must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_NULLABLE_REPEAT, caught.getErrorCode(), "Error code must be GRAMMAR_NULLABLE_REPEAT");
    }

    public static void testUndefinedTokenRejected() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {"token": "NonExistentToken"}
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Undefined token must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_UNDEFINED_TOKEN, caught.getErrorCode(), "Error code must be GRAMMAR_UNDEFINED_TOKEN");
    }

    public static void testUndefinedReferenceRejected() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {"ref": "NonExistentProduction"}
          }
        }
        """;
        GrammarException caught = null;
        try {
            GrammarLoader.loadFromJson(wrapGrammar(grammar));
        } catch (GrammarException e) {
            caught = e;
        }
        assertTrue(caught != null, "Undefined reference must throw GrammarException");
        assertEquals(GrammarException.GRAMMAR_UNDEFINED_REFERENCE, caught.getErrorCode(), "Error code must be GRAMMAR_UNDEFINED_REFERENCE");
    }

    public static void testAstKindAndCategoryMetadata() {
        String grammar = """
        {
          "start": "Root",
          "productions": {
            "Root": {
              "sequence": [{"token": "A"}, {"token": "B"}],
              "astKind": "BinaryExpr",
              "category": "expression"
            }
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(wrapGrammar(grammar));
        Production root = def.getProduction(0);
        assertEquals("BinaryExpr", root.astKind, "astKind populated");
        assertEquals(CstCategory.EXPRESSION, root.category, "Category expression populated");
    }

    public static void testContextualLexerAndModes() {
        String json = """
        {
          "formatVersion": 2,
          "name": "lexer_test", "version": 2, "caseSensitivity": true,
          "defaultFileExtensions": ["txt"], "defaultTextEncoding": "utf-8",
          "otherTextInside": true,
          "lexer": {
            "initialMode": "default",
            "tokens": {
              "A": {"regex": "a", "priority": 10, "pushMode": "submode"},
              "B": {"regex": "b", "popMode": true}
            },
            "trivia": {
              "Whitespace": {"regex": "\\\\s+"}
            },
            "modes": {
              "default": {"tokens": ["A"], "trivia": ["Whitespace"]},
              "submode": {"tokens": ["B"], "trivia": ["Whitespace"]}
            },
            "goals": {
              "RegExp": {"A": "B"}
            }
          },
          "grammar": {
            "start": "Root",
            "productions": {"Root": {"choice": [{"token": "A"}, {"token": "B"}]}}
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(json);
        assertTrue(def != null, "Contextual lexer loaded");
        assertEquals("default", def.initialLexerMode, "Initial mode is default");
        assertEquals(2, def.lexerModes.size(), "Two modes defined");
        assertTrue(def.lexerModes.containsKey("default"), "default mode exists");
        assertTrue(def.lexerModes.containsKey("submode"), "submode exists");

        assertTrue(def.lexerGoals.containsKey("RegExp"), "RegExp goal exists");
        LexerGoal goal = def.lexerGoals.get("RegExp");
        assertEquals(1, goal.mappings.size(), "RegExp has 1 mapping");
    }

    public static void testOperatorsTableLoading() {
        String json = """
        {
          "formatVersion": 2,
          "name": "operators_test", "version": 2, "caseSensitivity": true,
          "defaultFileExtensions": ["txt"], "defaultTextEncoding": "utf-8",
          "otherTextInside": true,
          "tokens": {
            "Plus": {"type": "SimpleToken", "startRegex": "\\\\+"},
            "Question": {"type": "SimpleToken", "startRegex": "\\\\?"},
            "Colon": {"type": "SimpleToken", "startRegex": ":"},
            "Num": {"type": "SimpleToken", "startRegex": "[0-9]+"}
          },
          "startTokens": ["Plus", "Question", "Colon", "Num"],
          "operators": [
            {
              "token": "Plus",
              "roles": ["prefix", "infix"],
              "prefixPrecedence": 10,
              "infixPrecedence": 5,
              "associativity": "left"
            },
            {
              "token": "Question",
              "role": "ternary",
              "precedence": 2,
              "middleTerminator": "Colon"
            }
          ],
          "grammar": {
            "start": "Root",
            "productions": {"Root": {"token": "Num"}}
          }
        }
        """;
        GrammarDefinition def = GrammarLoader.loadFromJson(json);
        assertTrue(def != null, "Operators loaded");
        assertEquals(3, def.operators.size(), "3 operator definitions (2 for Plus, 1 for Question)");

        OperatorDef op0 = def.operators.get(0);
        assertEquals(OperatorRole.PREFIX, op0.role, "First operator is prefix");
        assertEquals(10, op0.precedence, "Prefix precedence is 10");

        OperatorDef op1 = def.operators.get(1);
        assertEquals(OperatorRole.INFIX, op1.role, "Second operator is infix");
        assertEquals(5, op1.precedence, "Infix precedence is 5");

        OperatorDef op2 = def.operators.get(2);
        assertEquals(OperatorRole.TERNARY, op2.role, "Third operator is ternary");
        assertEquals(2, op2.precedence, "Ternary precedence is 2");
        assertTrue(op2.secondaryTokenId >= 0, "Secondary token ID resolved for Colon");
    }

    public static void testLoadJsonDefinitionFile() {
        File f = new File("definitions/json_definition.json");
        if (!f.exists()) {
            System.out.println("Skipping testLoadJsonDefinitionFile (not in root dir)");
            return;
        }
        try {
            GrammarDefinition def = GrammarLoader.loadFromFile(f.getAbsolutePath());
            assertTrue(def != null, "json_definition.json loaded");
            assertTrue(def.productions.size() > 0, "json_definition has productions");
            assertTrue(def.startProduction >= 0, "json_definition has valid start production");
        } catch (Exception e) {
            throw new AssertionError("Failed to load json_definition.json: " + e.getMessage(), e);
        }
    }

    public static void testLoadHtmlDefinitionFile() {
        File f = new File("definitions/html_definition.json");
        if (!f.exists()) {
            System.out.println("Skipping testLoadHtmlDefinitionFile (not in root dir)");
            return;
        }
        try {
            GrammarDefinition def = GrammarLoader.loadFromFile(f.getAbsolutePath());
            assertTrue(def != null, "html_definition.json loaded");
            assertTrue(def.productions.size() > 0, "html_definition has productions");

            // Verify astKind on PairedElement and SelfClosingElement
            Production pairedElem = def.getProductionByName("PairedElement");
            assertTrue(pairedElem != null, "PairedElement production exists");
            assertEquals("Element", pairedElem.astKind, "PairedElement astKind is Element");

            Production selfClosing = def.getProductionByName("SelfClosingElement");
            assertTrue(selfClosing != null, "SelfClosingElement production exists");
            assertEquals("SelfClosingTag", selfClosing.astKind, "SelfClosingElement astKind is SelfClosingTag");
        } catch (Exception e) {
            throw new AssertionError("Failed to load html_definition.json: " + e.getMessage(), e);
        }
    }

    public static int runAllTests() {
        int passed = 0;
        int failed = 0;

        Runnable[] tests = {
            GrammarLoaderTest::testLoadsFlattenResolvesAndValidates,
            GrammarLoaderTest::testOneOrMoreDesugaring,
            GrammarLoaderTest::testRejectDirectLeftRecursion,
            GrammarLoaderTest::testRejectIndirectLeftRecursion,
            GrammarLoaderTest::testAcceptRightRecursion,
            GrammarLoaderTest::testRejectNullableRepeat,
            GrammarLoaderTest::testRejectNullablePrattPostfix,
            GrammarLoaderTest::testUndefinedTokenRejected,
            GrammarLoaderTest::testUndefinedReferenceRejected,
            GrammarLoaderTest::testAstKindAndCategoryMetadata,
            GrammarLoaderTest::testContextualLexerAndModes,
            GrammarLoaderTest::testOperatorsTableLoading,
            GrammarLoaderTest::testLoadJsonDefinitionFile,
            GrammarLoaderTest::testLoadHtmlDefinitionFile
        };

        String[] names = {
            "testLoadsFlattenResolvesAndValidates",
            "testOneOrMoreDesugaring",
            "testRejectDirectLeftRecursion",
            "testRejectIndirectLeftRecursion",
            "testAcceptRightRecursion",
            "testRejectNullableRepeat",
            "testRejectNullablePrattPostfix",
            "testUndefinedTokenRejected",
            "testUndefinedReferenceRejected",
            "testAstKindAndCategoryMetadata",
            "testContextualLexerAndModes",
            "testOperatorsTableLoading",
            "testLoadJsonDefinitionFile",
            "testLoadHtmlDefinitionFile"
        };

        for (int i = 0; i < tests.length; i++) {
            try {
                tests[i].run();
                passed++;
                System.out.println("[PASS] GrammarLoaderTest." + names[i]);
            } catch (Throwable t) {
                failed++;
                System.err.println("[FAIL] GrammarLoaderTest." + names[i] + ": " + t.getMessage());
                t.printStackTrace();
            }
        }

        if (failed > 0) {
            throw new RuntimeException("GrammarLoaderTest: " + failed + " tests failed");
        }
        return passed;
    }
}
