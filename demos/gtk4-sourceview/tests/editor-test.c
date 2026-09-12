#define main example_main
#include "../main.c"
#undef main

static void check_color(Editor *editor, const char *needle)
{
    GtkTextIter begin, end;
    gtk_text_buffer_get_bounds(editor->buffer, &begin, &end);
    char *text = gtk_text_buffer_get_text(editor->buffer, &begin, &end, TRUE);
    char *match = strstr(text, needle);
    g_assert_nonnull(match);
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &begin,
                                      g_utf8_pointer_to_offset(text, match));
    GSList *tags = gtk_text_iter_get_tags(&begin);
    g_assert_nonnull(tags);
    g_slist_free(tags);
    g_free(text);
}

static void check_sync(Editor *editor)
{
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(editor->buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(editor->buffer, &start, &end, TRUE);
    g_assert_cmpuint(textparser_get_text_size(editor->parser), ==, strlen(text));
    g_assert_cmpmem(textparser_get_text(editor->parser), strlen(text), text, strlen(text));
    g_free(text);
}

static void append_tree(GString *tree, const textparser_token_item *token)
{
    for (; token != NULL; token = token->next) {
        g_string_append_printf(tree, "(%d,%zu,%u,%u,%u,%s", token->token_id,
            token->len, token->text_color, token->text_background, token->text_flags,
            token->error != NULL ? token->error : "");
        append_tree(tree, token->child);
        g_string_append_c(tree, ')');
    }
}

static void check_full_equivalence(Editor *editor)
{
    textparser_t full = NULL;
    g_assert_cmpint(textparser_openmem(textparser_get_text(editor->parser),
        textparser_get_text_size(editor->parser), TEXTPARSER_ENCODING_UTF_8, &full), ==, 0);
    int result = textparser_parse(full, editor->definition);
    GString *actual = g_string_new(NULL), *expected = g_string_new(NULL);
    append_tree(actual, textparser_get_first_token(editor->parser));
    append_tree(expected, textparser_get_first_token(full));
    if (strcmp(actual->str, expected->str) != 0)
        g_printerr("Mismatching text: %s\n", textparser_get_text(editor->parser));
    g_assert_cmpstr(actual->str, ==, expected->str);
    g_assert_cmpint(editor->parse_result, ==, result);
    g_assert_cmpstr(textparser_parse_error(editor->parser), ==, textparser_parse_error(full));
    if (result != 0)
        g_assert_cmpuint(textparser_parse_error_position(editor->parser), ==,
                         textparser_parse_error_position(full));
    g_string_free(actual, TRUE);
    g_string_free(expected, TRUE);
    textparser_close(full);
}

static void check_edit_matrix(void)
{
    Editor safe = {.definition = &json_definition};
    g_assert_cmpint(textparser_openmem("{\"n\": 12}", -1,
        TEXTPARSER_ENCODING_UTF_8, &safe.parser), ==, 0);
    safe.parse_result = textparser_parse(safe.parser, safe.definition);
    parse_edit(&safe, 6, 1, "3", 1);
    g_assert_cmpuint(safe.full_parse_fallbacks, ==, 0);
    check_full_equivalence(&safe);
    textparser_close(safe.parser);
    const char *documents[] = {
        "{\"key\": \"hello\", \"n\": 12}",
        "{\"é😀\": [true, false, null, 123.45]}",
        "{\"x\": {\"y\": \"a\\nb\"}}", "{}", "", "{\"x\": 12"
    };
    const char *insertions[] = {"", "x", "3", " ", "\"", "\\", "}", ":", "😀"};
    for (size_t d = 0; d < G_N_ELEMENTS(documents); d++) {
        size_t length = strlen(documents[d]);
        for (size_t offset = 0; offset <= length; offset++) {
            if ((documents[d][offset] & 0xc0) == 0x80) continue;
            for (int remove = 0; remove <= (offset < length ? 1 : 0); remove++) {
                size_t removed = remove ? g_utf8_next_char(documents[d] + offset) -
                                          (documents[d] + offset) : 0;
                for (size_t i = 0; i < G_N_ELEMENTS(insertions); i++) {
                    Editor editor = {.definition = &json_definition};
                    g_assert_cmpint(textparser_openmem(documents[d], -1,
                        TEXTPARSER_ENCODING_UTF_8, &editor.parser), ==, 0);
                    editor.parse_result = textparser_parse(editor.parser, editor.definition);
                    parse_edit(&editor, offset, removed, insertions[i], strlen(insertions[i]));
                    check_full_equivalence(&editor);
                    textparser_close(editor.parser);
                }
            }
        }
    }
}

static void check_json_definition(void)
{
    const char *valid[] = {
        "{}", "[]", "true", "false", "null", "-2.5E+3", "\"hello\"",
        "{\"é😀\": [true, null, -12, 1.25e-2]}",
        "{\"a\\\"b\": \"quote: \\\" slash: \\\\ unicode: \\uABCD\"}"
    };
    const char *invalid[] = {
        "{\"x\": @}", "[oops]", "garbage {}", "{} garbage",
        "{\"x\": TRUE}", "{\"x\": 01}", "{\"x\": 1.}",
        "{\"x\": \"bad\\q\"}", "{\"x\": \"bad\ntext\"}"
    };
    for (int group = 0; group < 2; group++) {
        const char **cases = group == 0 ? valid : invalid;
        size_t count = group == 0 ? G_N_ELEMENTS(valid) : G_N_ELEMENTS(invalid);
        for (size_t i = 0; i < count; i++) {
            textparser_t parser = NULL;
            g_assert_cmpint(textparser_openmem(cases[i], -1,
                TEXTPARSER_ENCODING_UTF_8, &parser), ==, 0);
            int result = textparser_parse(parser, &json_definition);
            gboolean has_error = result != 0 || textparser_parse_error(parser) != NULL ||
                find_unprocessed(textparser_get_first_token(parser)) != NULL;
            g_assert_cmpint(has_error, ==, group == 1);
            textparser_close(parser);
        }
    }
}

int main(void)
{
    check_json_definition();
    check_edit_matrix();
    GtkApplication *app = gtk_application_new("com.example.TextparserTest", G_APPLICATION_NON_UNIQUE);
    GError *error = NULL;
    g_assert_true(g_application_register(G_APPLICATION(app), NULL, &error));
    activate(app, NULL);
    GtkWindow *window = gtk_application_get_active_window(app);
    g_assert_nonnull(window);
    Editor *editor = g_object_get_data(G_OBJECT(window), "editor");
    const char *bad_json =
        "{\n"
        "  \"message\": \"Hello, свет!\",\n"
        "  \"count\": 42,\n"
        "  \"enabled\": true,\n"
        "  \"items\": [1, 2, null],asdsadasd\n"
        "}\n";
    gtk_text_buffer_set_text(editor->buffer, bad_json, -1);
    g_assert_cmpuint(textparser_parse_error_position(editor->parser), ==, 93);
    g_assert_cmpuint(editor->error_count, >, 0);
    GtkTextIter error_start, error_end;
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &error_start,
        g_utf8_pointer_to_offset(bad_json, strstr(bad_json, "asdsadasd")));
    error_end = error_start;
    for (int i = 0; i < 9; i++) {
        g_assert_true(gtk_text_iter_has_tag(&error_end, editor->error_tag));
        gtk_text_iter_forward_char(&error_end);
    }
    g_assert_false(gtk_text_iter_has_tag(&error_end, editor->error_tag));
    gtk_text_iter_forward_char(&error_end);
    g_assert_false(gtk_text_iter_has_tag(&error_end, editor->error_tag));
    gtk_text_iter_backward_char(&error_start);
    g_assert_false(gtk_text_iter_has_tag(&error_start, editor->error_tag));
    g_assert_cmpuint(point_error_length("ошибка😀,}", 0), ==, strlen("ошибка😀"));
    g_assert_cmpuint(point_error_length("}", 0), ==, 0);
    g_assert_cmpuint(point_error_length("", 0), ==, 0);
    gtk_text_buffer_set_text(editor->buffer, "{} garbage", -1);
    g_assert_cmpuint(editor->error_count, >, 0);
    GtkTextIter garbage;
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &garbage, 3);
    g_assert_true(gtk_text_iter_has_tag(&garbage, editor->error_tag));
    g_assert_nonnull(strstr(gtk_label_get_text(editor->diagnostic), "Unexpected text"));
    gtk_text_buffer_set_text(editor->buffer,
        "{\"message\": \"свет\", \"count\": 42, \"enabled\": true}", -1);
    g_assert_cmpuint(editor->error_count, ==, 0);
    check_color(editor, "свет");
    check_color(editor, "42");
    check_color(editor, "true");
    const char *cases[] = {"{\"é😀\": [false, 123]}", "{\"unfinished\": ", "", "{\"ok\": null}"};
    for (size_t i = 0; i < G_N_ELEMENTS(cases); i++) {
        gtk_text_buffer_set_text(editor->buffer, cases[i], -1);
        if (i == 0) check_color(editor, "123");
        if (i == 3) check_color(editor, "null");
    }
    gtk_text_buffer_set_text(editor->buffer, "{\"é😀\": 12", -1);
    g_assert_cmpuint(editor->error_count, >, 0);
    g_assert_true(gtk_widget_get_visible(GTK_WIDGET(editor->diagnostic)));
    g_assert_nonnull(strstr(gtk_label_get_text(editor->diagnostic), "Line 1, column 10:"));
    GtkTextIter last;
    gtk_text_buffer_get_end_iter(editor->buffer, &last);
    gtk_text_iter_backward_char(&last);
    g_assert_true(gtk_text_iter_has_tag(&last, editor->error_tag));
    PangoUnderline underline;
    g_object_get(editor->error_tag, "underline", &underline, NULL);
    g_assert_cmpint(underline, ==, PANGO_UNDERLINE_ERROR);
    gtk_text_buffer_set_text(editor->buffer, "{}", -1);
    g_assert_cmpuint(editor->error_count, ==, 0);
    g_assert_false(gtk_widget_get_visible(GTK_WIDGET(editor->diagnostic)));
    gtk_text_buffer_get_start_iter(editor->buffer, &last);
    g_assert_false(gtk_text_iter_has_tag(&last, editor->error_tag));
    gtk_text_buffer_set_text(editor->buffer, "{}", -1);
    /* Regression: editing string contents used to move the closing-quote tag. */
    gtk_text_buffer_set_text(editor->buffer, "{\"key\": \"hello\", \"n\": 12}", -1);
    GtkTextIter string_position;
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &string_position, 10);
    guint fallbacks = editor->full_parse_fallbacks;
    gtk_text_buffer_insert(editor->buffer, &string_position, "X", 1);
    g_assert_cmpuint(editor->full_parse_fallbacks, >, fallbacks);
    check_full_equivalence(editor);
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &string_position, 10);
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(editor->buffer);
    g_assert_true(gtk_text_iter_has_tag(&string_position,
        gtk_text_tag_table_lookup(table, "textparser-ce9178")));
    g_assert_false(gtk_text_iter_has_tag(&string_position,
        gtk_text_tag_table_lookup(table, "textparser-9e6a57")));
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &string_position, 15);
    g_assert_true(gtk_text_iter_has_tag(&string_position,
        gtk_text_tag_table_lookup(table, "textparser-9e6a57")));
    gtk_text_buffer_set_text(editor->buffer, "{}", -1);
    textparser_t original_parser = editor->parser;
    gtk_text_buffer_set_enable_undo(editor->buffer, TRUE);
    GtkTextIter location, deletion_end;
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &location, 1);
    gtk_text_buffer_insert(editor->buffer, &location, "\"é😀\": 12", -1);
    check_sync(editor);
    check_full_equivalence(editor);
    check_color(editor, "12");
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &location, 3);
    deletion_end = location;
    gtk_text_iter_forward_char(&deletion_end);
    gtk_text_buffer_delete(editor->buffer, &location, &deletion_end);
    check_sync(editor);
    check_full_equivalence(editor);
    g_assert_true(gtk_text_buffer_get_can_undo(editor->buffer));
    gtk_text_buffer_undo(editor->buffer);
    check_sync(editor);
    check_full_equivalence(editor);
    g_assert_true(gtk_text_buffer_get_can_redo(editor->buffer));
    gtk_text_buffer_redo(editor->buffer);
    check_sync(editor);
    check_full_equivalence(editor);
    gtk_text_buffer_get_end_iter(editor->buffer, &deletion_end);
    location = deletion_end;
    gtk_text_iter_backward_char(&location);
    gtk_text_buffer_delete(editor->buffer, &location, &deletion_end);
    check_sync(editor);
    check_full_equivalence(editor);
    g_assert_cmpuint(editor->error_count, >, 0);
    check_full_equivalence(editor);
    gtk_text_buffer_get_end_iter(editor->buffer, &location);
    gtk_text_buffer_insert(editor->buffer, &location, "}", 1);
    check_sync(editor);
    check_full_equivalence(editor);
    g_assert_cmpuint(editor->error_count, ==, 0);
    g_assert_true(editor->parser == original_parser);
    gtk_window_destroy(window);
    g_object_unref(app);
    return 0;
}
