#include <gtest/gtest.h>
#include <textparser.hpp>
#include <textparser-json.h>

#include <string>
#include <fstream>
#include <sstream>

TEST(schema_parity, json_definition_bom_parity) {
    const char *json_text = R"({
        "name": "parity_test_lang",
        "version": 2.0,
        "caseSensitivity": true,
        "defaultFileExtensions": ["test"],
        "defaultTextEncoding": "utf-8",
        "supportedBom": "utf-8, utf-16-le",
        "startTokens": ["MainToken"],
        "tokens": {
            "MainToken": {
                "type": "SimpleToken",
                "startRegex": "[a-zA-Z]+"
            }
        }
    })";

    textparser_language_definition *def = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_text, &def);
    ASSERT_EQ(err, TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(def, nullptr);

    EXPECT_STREQ(def->name, "parity_test_lang");
    EXPECT_DOUBLE_EQ(def->version, 2.0);
    EXPECT_EQ(def->default_text_encoding, TEXTPARSER_ENCODING_UTF_8);
    EXPECT_EQ(def->supported_bom, (TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_16_LE));

    textparser_free_language_definition(def);
}

TEST(schema_parity, json_definition_array_bom_parity) {
    const char *json_text = R"({
        "name": "parity_test_array_bom",
        "version": 1.0,
        "caseSensitivity": false,
        "defaultFileExtensions": ["test2"],
        "defaultTextEncoding": "utf-8",
        "supportedBom": ["utf-8", "utf-32-be"],
        "startTokens": ["TokenA"],
        "tokens": {
            "TokenA": {
                "type": "SimpleToken",
                "startRegex": "foo"
            }
        }
    })";

    textparser_language_definition *def = nullptr;
    int err = textparser_json_load_language_definition_from_string(json_text, &def);
    ASSERT_EQ(err, TEXTPARSER_JSON_NO_ERROR);
    ASSERT_NE(def, nullptr);

    EXPECT_EQ(def->supported_bom, (TEXTPARSER_BOM_UTF_8 | TEXTPARSER_BOM_UTF_32_BE));

    textparser_free_language_definition(def);
}
