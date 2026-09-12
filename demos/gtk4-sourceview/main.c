#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include "json.json.h"
#include <string.h>

typedef struct {
    GtkTextBuffer *buffer;
    const textparser_language_definition *definition;
    GtkTextTag *error_tag;
    GtkLabel *diagnostic;
    guint error_count;
    textparser_t parser;
    int parse_result;
    guint full_parse_fallbacks;
    GtkCssProvider *zoom_css;
    GdkDisplay *display;
    int font_size;
} Editor;

static void
set_font_size(Editor *editor, int size)
{
    editor->font_size = CLAMP(size, 6, 48);
    char css[80];
    g_snprintf(css, sizeof css, ".textparser-editor { font-size: %dpt; }",
               editor->font_size);
    gtk_css_provider_load_from_string(editor->zoom_css, css);
}

static gboolean
zoom_scroll(GtkEventControllerScroll *controller, double dx, double dy, gpointer data)
{
    (void)dx;
    GdkModifierType modifiers = gtk_event_controller_get_current_event_state(
        GTK_EVENT_CONTROLLER(controller));
    if (!(modifiers & GDK_CONTROL_MASK) || dy == 0)
        return FALSE;

    Editor *editor = data;
    set_font_size(editor, editor->font_size + (dy < 0 ? 1 : -1));
    return TRUE;
}

static const textparser_token_item *
find_unprocessed(const textparser_token_item *token)
{
    for (; token != NULL; token = token->next) {
        if (token->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED && token->len > 0)
            return token;
        const textparser_token_item *child = find_unprocessed(token->child);
        if (child != NULL)
            return child;
    }
    return NULL;
}

static void
show_error(Editor *editor, const char *text, size_t position, size_t length,
           const char *message)
{
    size_t bytes = strlen(text);
    position = MIN(position, bytes);
    size_t finish = position + MIN(length, bytes - position);
    /* Clamp byte offsets to whole UTF-8 characters. */
    while (position > 0 && (text[position] & 0xc0) == 0x80)
        position--;
    while (finish < bytes && (text[finish] & 0xc0) == 0x80)
        finish++;

    GtkTextIter start, end;
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &start,
                                      g_utf8_pointer_to_offset(text, text + position));
    gtk_text_buffer_get_iter_at_offset(editor->buffer, &end,
                                      g_utf8_pointer_to_offset(text, text + finish));
    if (editor->error_count++ == 0) {
        char *description = g_strdup_printf("Line %d, column %d: %s",
            gtk_text_iter_get_line(&start) + 1,
            gtk_text_iter_get_line_offset(&start) + 1, message);
        gtk_label_set_text(editor->diagnostic, description);
        g_free(description);
    }
    if (gtk_text_iter_equal(&start, &end)) {
        if (!gtk_text_iter_is_end(&end))
            gtk_text_iter_forward_char(&end);
        else
            gtk_text_iter_backward_char(&start);
    }
    gtk_text_buffer_apply_tag(editor->buffer, editor->error_tag, &start, &end);
}

static void
highlight_errors(Editor *editor, const textparser_token_item *token, const char *text)
{
    for (; token != NULL; token = token->next) {
        const char *message = textparser_get_token_error(token);
        if (token->token_id == TEXTPARSER_TOKEN_ID_UNPROCESSED && token->len > 0)
            message = "Unexpected text: no JSON token matched";
        if (message != NULL || token->token_id == TEXTPARSER_TOKEN_ID_ERROR)
            show_error(editor, text, textparser_get_token_position(token),
                       textparser_get_token_length(token),
                       message != NULL ? message : "Unexpected token");
        highlight_errors(editor, token->child, text);
    }
}

static void
highlight_tokens(Editor *editor, const textparser_token_item *token,
                 const char *text, size_t text_length)
{
    for (; token != NULL; token = token->next) {
        size_t position = textparser_get_token_position(token);
        size_t length = textparser_get_token_length(token);
        uint32_t color = textparser_get_token_text_color(token);
        if (color != TEXTPARSER_NOCOLOR && position <= text_length &&
            length <= text_length - position && length > 0 &&
            g_utf8_validate(text, position, NULL) &&
            g_utf8_validate(text + position, length, NULL)) {
            char name[32];
            char foreground[8];
            g_snprintf(name, sizeof name, "textparser-%06x", color & 0xffffff);
            g_snprintf(foreground, sizeof foreground, "#%06x", color & 0xffffff);
            GtkTextTag *tag = gtk_text_tag_table_lookup(
                gtk_text_buffer_get_tag_table(editor->buffer), name);
            if (tag == NULL)
                tag = gtk_text_buffer_create_tag(editor->buffer, name,
                                                "foreground", foreground, NULL);
            GtkTextIter start, end;
            gtk_text_buffer_get_iter_at_offset(editor->buffer, &start,
                                              g_utf8_pointer_to_offset(text, text + position));
            gtk_text_buffer_get_iter_at_offset(editor->buffer, &end,
                                              g_utf8_pointer_to_offset(text, text + position + length));
            /* Child styles replace their container's foreground. */
            gtk_text_buffer_remove_all_tags(editor->buffer, &start, &end);
            gtk_text_buffer_apply_tag(editor->buffer, tag, &start, &end);
        }
        highlight_tokens(editor, token->child, text, text_length);
    }
}

/* Handle-level errors only carry a byte position. Infer a display span for
 * unexpected text; actual token errors retain their parser-provided lengths. */
static size_t
point_error_length(const char *text, size_t position)
{
    size_t bytes = strlen(text);
    if (position >= bytes || (text[position] & 0xc0) == 0x80)
        return 0;
    const char *start = text + position;
    const char *end = start;
    while (*end != '\0' && !g_unichar_isspace(g_utf8_get_char(end)) &&
           strchr("{}[],:\"", *end) == NULL)
        end = g_utf8_next_char(end);
    return (size_t)(end - start);
}

static void
highlight(Editor *editor)
{
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(editor->buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(editor->buffer, &start, &end, TRUE);
    gtk_text_buffer_remove_all_tags(editor->buffer, &start, &end);
    editor->error_count = 0;
    gtk_label_set_text(editor->diagnostic, "");

    highlight_tokens(editor, textparser_get_first_token(editor->parser), text, strlen(text));
    /* Apply diagnostics after colors so container styling cannot erase them. */
    highlight_errors(editor, textparser_get_first_token(editor->parser), text);
    const char *message = textparser_parse_error(editor->parser);
    if (editor->parse_result != TEXTPARSER_OK || message != NULL) {
        size_t position = textparser_parse_error_position(editor->parser);
        show_error(editor, text, position, point_error_length(text, position),
                   message != NULL ? message : "Parsing failed");
    }
    gtk_widget_set_visible(GTK_WIDGET(editor->diagnostic), editor->error_count > 0);
    g_free(text);
}

static size_t
byte_offset(GtkTextBuffer *buffer, const GtkTextIter *position)
{
    GtkTextIter start;
    gtk_text_buffer_get_start_iter(buffer, &start);
    char *prefix = gtk_text_buffer_get_text(buffer, &start, position, TRUE);
    size_t offset = strlen(prefix);
    g_free(prefix);
    return offset;
}

/* Copy token IDs and starts, not pointers: an incremental parse can free nodes.
 * Include the ancestor chain of the character just before the edit endpoint. */
static GArray *
endpoint_state(textparser_t parser, size_t endpoint)
{
    GArray *state = g_array_new(FALSE, FALSE, sizeof(size_t));
    if (endpoint == 0)
        return state;
    size_t position = endpoint - 1;
    size_t offset = 0;
    const textparser_token_item *token = textparser_get_first_token(parser);
    while (token != NULL) {
        if (position >= offset && position - offset < token->len) {
            size_t id = (size_t)token->token_id;
            g_array_append_val(state, id);
            g_array_append_val(state, offset);
            token = token->child;
        } else {
            offset += token->len;
            token = token->next;
        }
    }
    return state;
}

static gboolean
tokens_consistent(const textparser_token_item *token, size_t expected_length,
                  const textparser_language_definition *definition)
{
    size_t total = 0;
    for (; token != NULL; token = token->next) {
        if (token->len > expected_length - total)
            return FALSE;
        total += token->len;
        if (token->child != NULL &&
            !tokens_consistent(token->child, token->len, definition))
            return FALSE;
        if (token->token_id >= 0 &&
            definition->tokens[token->token_id].type == TEXTPARSER_TOKEN_TYPE_START_STOP) {
            const textparser_token_item *last = token->child;
            if (last == NULL || last->token_id != TEXTPARSER_TOKEN_ID_START_DELIMITER)
                return FALSE;
            while (last->next != NULL)
                last = last->next;
            if (last->token_id != TEXTPARSER_TOKEN_ID_END_DELIMITER)
                return FALSE;
        }
    }
    return total == expected_length;
}

static gboolean
contains_json_syntax(const char *text, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        if (strchr("\"\\{}[]:,", text[i]) != NULL)
            return TRUE;
    }
    return FALSE;
}

static void
parse_edit(Editor *editor, size_t offset, size_t removed,
           const char *inserted, size_t added)
{
    GArray *before = endpoint_state(editor->parser, offset + removed);
    const char *old_text = textparser_get_text(editor->parser);
    gboolean fallback = editor->parse_result != TEXTPARSER_OK ||
        textparser_parse_error(editor->parser) != NULL ||
        find_unprocessed(textparser_get_first_token(editor->parser)) != NULL ||
        contains_json_syntax(old_text + offset, removed) ||
        contains_json_syntax(inserted, added);

    editor->parse_result = textparser_parse_incremental(
        editor->parser, editor->definition, offset, removed, inserted, added, NULL);
    GArray *after = endpoint_state(editor->parser, offset + added);
    fallback = fallback || before->len != after->len ||
        (before->len > 0 && memcmp(before->data, after->data,
                                  before->len * sizeof(size_t)) != 0) ||
        editor->parse_result != TEXTPARSER_OK ||
        textparser_parse_error(editor->parser) != NULL ||
        find_unprocessed(textparser_get_first_token(editor->parser)) != NULL ||
        !tokens_consistent(textparser_get_first_token(editor->parser),
                           textparser_get_text_size(editor->parser), editor->definition);
    g_array_unref(before);
    g_array_unref(after);
    if (fallback) {
        /* Reset the tree by fully parsing the already updated parser text. */
        editor->parse_result = textparser_parse(editor->parser, editor->definition);
        editor->full_parse_fallbacks++;
    }
}

/* These handlers run before GTK mutates the buffer, so offsets describe the
 * parser's old text. The changed signal repaints after GTK applies the edit. */
static void
text_inserted(GtkTextBuffer *buffer, GtkTextIter *location,
              char *text, int length, gpointer data)
{
    Editor *editor = data;
    parse_edit(editor, byte_offset(buffer, location), 0,
               text, length < 0 ? strlen(text) : (size_t)length);
}

static void
text_deleted(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end,
             gpointer data)
{
    Editor *editor = data;
    size_t offset = byte_offset(buffer, start);
    size_t end_offset = byte_offset(buffer, end);
    parse_edit(editor, offset, end_offset - offset, NULL, 0);
}

static void
buffer_changed(GtkTextBuffer *buffer, gpointer data)
{
    (void)buffer;
    highlight(data);
}

static void
editor_free(gpointer data)
{
    Editor *editor = data;
    textparser_close(editor->parser);
    g_signal_handlers_disconnect_by_data(editor->buffer, editor);
    g_object_unref(editor->buffer);
    gtk_style_context_remove_provider_for_display(editor->display,
                                                  GTK_STYLE_PROVIDER(editor->zoom_css));
    g_object_unref(editor->zoom_css);
    g_object_unref(editor->display);
    g_free(editor);
}

static void
activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = gtk_application_get_active_window(app)
        ? GTK_WIDGET(gtk_application_get_active_window(app)) : NULL;
    if (window != NULL) {
        gtk_window_present(GTK_WINDOW(window));
        return;
    }

    Editor *editor = g_new0(Editor, 1);
    editor->definition = &json_definition;
    int result = textparser_openmem("", 0, TEXTPARSER_ENCODING_UTF_8, &editor->parser);
    if (result != TEXTPARSER_OK) {
        g_printerr("Cannot create parser: %s\n", textparser_strerror(result));
        g_free(editor);
        g_application_quit(G_APPLICATION(app));
        return;
    }
    editor->parse_result = textparser_parse(editor->parser, editor->definition);

    window = gtk_application_window_new(app);
    g_object_set_data_full(G_OBJECT(window), "editor", editor, editor_free);
    gtk_window_set_title(GTK_WINDOW(window), "JSON — textparser");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *view = gtk_source_view_new();
    gtk_widget_add_css_class(view, "textparser-editor");
    editor->display = g_object_ref(gtk_widget_get_display(view));
    editor->zoom_css = gtk_css_provider_new();
    gtk_style_context_add_provider_for_display(editor->display,
        GTK_STYLE_PROVIDER(editor->zoom_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    set_font_size(editor, 12);
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view), 4);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    editor->buffer = g_object_ref(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
    editor->error_tag = gtk_text_buffer_create_tag(editor->buffer, "parser-error",
        "underline", PANGO_UNDERLINE_ERROR,
        "underline-rgba", &(GdkRGBA){1.0, 0.25, 0.25, 1.0}, NULL);
    editor->diagnostic = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_xalign(editor->diagnostic, 0.0);
    gtk_label_set_wrap(editor->diagnostic, TRUE);
    gtk_label_set_selectable(editor->diagnostic, TRUE);
    gtk_widget_set_margin_start(GTK_WIDGET(editor->diagnostic), 8);
    gtk_widget_set_margin_end(GTK_WIDGET(editor->diagnostic), 8);
    gtk_widget_set_margin_top(GTK_WIDGET(editor->diagnostic), 6);
    gtk_widget_set_margin_bottom(GTK_WIDGET(editor->diagnostic), 6);
    gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(editor->buffer), FALSE);
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(
        gtk_source_style_scheme_manager_get_default(), "Adwaita-dark");
    gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(editor->buffer), scheme);
    g_signal_connect(editor->buffer, "insert-text", G_CALLBACK(text_inserted), editor);
    g_signal_connect(editor->buffer, "delete-range", G_CALLBACK(text_deleted), editor);
    g_signal_connect_after(editor->buffer, "changed", G_CALLBACK(buffer_changed), editor);
    gtk_text_buffer_set_text(editor->buffer,
        "{\n"
        "  \"message\": \"Hello, свет!\",\n"
        "  \"count\": 42,\n"
        "  \"enabled\": true,\n"
        "  \"items\": [1, 2, null]\n"
        "}\n", -1);


    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    GtkEventController *scroll = gtk_event_controller_scroll_new(
        GTK_EVENT_CONTROLLER_SCROLL_VERTICAL | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
    gtk_event_controller_set_propagation_phase(scroll, GTK_PHASE_CAPTURE);
    g_signal_connect(scroll, "scroll", G_CALLBACK(zoom_scroll), editor);
    gtk_widget_add_controller(scrolled_window, scroll);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), view);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(layout), scrolled_window);
    gtk_box_append(GTK_BOX(layout), GTK_WIDGET(editor->diagnostic));
    gtk_window_set_child(GTK_WINDOW(window), layout);
    gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new(
        "com.example.GtkSourceViewTextparser", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
