#include <textparser.h>
#include <cfml_definition.json.h>
//#include <json_definition.json.h>
//#include <php_definition.json.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


static void print_textparser_token_item(void *handle, textparser_token_item *item, int level)
{
    const language_definition *language;
    const char *token_name = NULL;
    char *token_text = NULL;

    for(int c = 0; c < level; c++)
        putc(' ', stdout);

    language = textparser_get_language(handle);
    token_name = textparser_get_token_id_name(handle, item->token_id);
    token_text = textparser_get_token_text(handle, item);

    uint32_t text_background = language->tokens[item->token_id].text_background;
    uint32_t text_color = language->tokens[item->token_id].text_color;
    uint32_t text_flags = language->tokens[item->token_id].text_flags;

    if ((token_text)&&((item->child == NULL)||(strlen(token_text) < 50))) {
        printf("type: \033[48;5;4m%s\033[0m, text: \033[48;5;5m%s\033[0m\n", token_name, token_text);
    } else {
        printf("type: \033[48;5;4m%s\033[0m\n", token_name);
    }

    free(token_text);

    struct textparser_token_item *child = item->child;
    while(child)
    {
        print_textparser_token_item(handle, child, level + 1);
        child = child->next;
    }
}

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    void *handle = NULL;

    if (argc != 2)
    {
        printf("Usage: textparser <file.cfml>\n");

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    int err = textparser_openfile(argv[1], ADV_REGEX_TEXT_LATIN1, &handle);
    if (err)
    {
        fprintf(stderr, "textparser_openfile returned with error code: %d\n", err);

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    err = textparser_parse(handle, &cfml_definition);
    if (err)
    {
        fprintf(stderr, "textparser_parse returned with error code: %d\n", err);

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    printf("File parsed ok!\n");

    textparser_token_item *item = textparser_get_first_token(handle);
    while (item)
    {
        print_textparser_token_item(handle, item, 0);

        item = item->next;
    }

cleanup:
    if(handle)
    {
        textparser_close(handle);
        handle = NULL;
    }

    return ret;
}
