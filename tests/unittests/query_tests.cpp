#include <gtest/gtest.h>
#include <textparser.h>
#include <string>
#include <vector>

#include <html_definition.json.h>

TEST(native_query_engine, type_selector) {
    const char *code = R"(
        <html>
            <head><title>Test</title></head>
            <body>
                <div class="container">
                    <h1>Title</h1>
                </div>
            </body>
        </html>
    )";

    textparser_t handle = NULL;
    int err = textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    ASSERT_EQ(err, 0);
    ASSERT_NE(handle, (void*)0);

    err = textparser_parse(handle, &html_definition);
    ASSERT_EQ(err, 0);

    // Query for all Tag nodes
    size_t count = 0;
    const textparser_token_item **results = textparser_query(handle, NULL, "Tag", &count);
    EXPECT_GT(count, 0u);
    EXPECT_NE(results, (void*)0);

    if (results) {
        for (size_t i = 0; i < count; i++) {
            const char *type_str = textparser_get_token_type_str(&html_definition, results[i]);
            EXPECT_STREQ(type_str, "Tag");
        }
        textparser_free_query_result(results);
    }

    textparser_close(handle);
}

TEST(native_query_engine, child_combinator) {
    const char *code = R"(
        <div class="test">
            <h1>Header</h1>
        </div>
    )";

    textparser_t handle = NULL;
    textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    textparser_parse(handle, &html_definition);

    size_t count = 0;
    // Tag > AttributeName (AttributeName inside Tag)
    const textparser_token_item **results = textparser_query(handle, NULL, "Tag > AttributeName", &count);
    EXPECT_GT(count, 0u);
    if (results) {
        textparser_free_query_result(results);
    }

    // Direct child that fails
    count = 0;
    results = textparser_query(handle, NULL, "AttributeName > Tag", &count);
    EXPECT_EQ(count, 0u);
    EXPECT_EQ(results, (void*)0);

    textparser_close(handle);
}

TEST(native_query_engine, descendant_combinator) {
    const char *code = R"(
        <html>
            <body>
                <div class="test">
                    <h1>Title</h1>
                </div>
            </body>
        </html>
    )";

    textparser_t handle = NULL;
    textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    textparser_parse(handle, &html_definition);

    size_t count = 0;
    // AttributeName is descendant of Tag
    const textparser_token_item **results = textparser_query(handle, NULL, "Tag AttributeName", &count);
    EXPECT_GT(count, 0u);
    if (results) {
        textparser_free_query_result(results);
    }

    textparser_close(handle);
}

TEST(native_query_engine, multi_query) {
    const char *code = R"(
        <div>
            <h1>Title</h1>
        </div>
    )";

    textparser_t handle = NULL;
    textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    textparser_parse(handle, &html_definition);

    size_t count = 0;
    const textparser_token_item **results = textparser_query(handle, NULL, "Tag, ClosingTag", &count);
    EXPECT_GT(count, 0u);
    if (results) {
        textparser_free_query_result(results);
    }

    textparser_close(handle);
}

TEST(native_query_engine, scoped_root_search) {
    const char *code = R"(
        <div class="box">
            <h1 id="t1">Title 1</h1>
        </div>
        <div class="box2">
            <h2 id="t2">Title 2</h2>
        </div>
    )";

    textparser_t handle = NULL;
    textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    textparser_parse(handle, &html_definition);

    // Find all Tag nodes first
    size_t tag_count = 0;
    const textparser_token_item **tag_nodes = textparser_query(handle, NULL, "Tag", &tag_count);
    ASSERT_GT(tag_count, 0u);
    ASSERT_NE(tag_nodes, (void*)0);

    // Search for AttributeName scoped only to tag_nodes[0]
    size_t scoped_count = 0;
    const textparser_token_item **scoped_results = textparser_query(handle, tag_nodes[0], "AttributeName", &scoped_count);
    EXPECT_GT(scoped_count, 0u);
    if (scoped_results) {
        textparser_free_query_result(scoped_results);
    }

    textparser_free_query_result(tag_nodes);
    textparser_close(handle);
}

TEST(native_query_engine, edge_cases) {
    const char *code = "<div></div>";
    textparser_t handle = NULL;
    textparser_openmem(code, (int)strlen(code), TEXTPARSER_ENCODING_UTF_8, &handle);
    textparser_parse(handle, &html_definition);

    size_t count = 100;
    // NULL handle
    EXPECT_EQ(textparser_query(NULL, NULL, "Tag", &count), (void*)0);
    EXPECT_EQ(count, 0u);

    // NULL selector
    EXPECT_EQ(textparser_query(handle, NULL, NULL, &count), (void*)0);
    EXPECT_EQ(count, 0u);

    // Empty selector
    EXPECT_EQ(textparser_query(handle, NULL, "", &count), (void*)0);
    EXPECT_EQ(count, 0u);

    // NULL out_count pointer
    EXPECT_EQ(textparser_query(handle, NULL, "Tag", NULL), (void*)0);

    // Unknown token name
    EXPECT_EQ(textparser_query(handle, NULL, "nonexistent_token_type", &count), (void*)0);
    EXPECT_EQ(count, 0u);

    // Free NULL results array gracefully
    textparser_free_query_result(NULL);

    textparser_close(handle);
}
