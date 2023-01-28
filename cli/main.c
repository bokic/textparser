#include <textparser.h>
#include <cfml_definition.h>
#include <json_definition.h>
#include <php_definition.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


_Static_assert(__STDC_VERSION__ == 202000, "Wrong C standard");

static void print_textparse_token_item(void *handle, textparse_token_item_t *item, int level)
{
    const char *token_name;
    char *token_text;

    for(int c = 0; c < level; c++)
        putc(' ', stdout);

    token_name = textparse_get_token_id_name(handle, item->token_id);
    token_text = textparse_get_token_text(handle, item);

    printf("type: \e[48;5;4m%s\e[0m, text: \e[48;5;5m%s\e[0m\n", token_name, token_text);

    if (item->child)
        print_textparse_token_item(handle, item->child, level + 1);

    free(token_text);
}

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    void *handle = NULL;
    int err = 0;

    if (argc != 2)
    {
        printf("Usage: textparser <file.json>\n");

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    err = textparse_openfile(argv[1], TEXTPARSE_LATIN_1, &handle);
    if (err)
    {
        fprintf(stderr, "textparse_openfile returned with error code: %d\n", err);

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    err = textparse_parse(handle, &json_definition);
    if (err)
    {
        fprintf(stderr, "textparse_parse returned with error code: %d\n", err);

        ret = EXIT_FAILURE;
        goto cleanup;
    }

    printf("File parsed ok!\n");

    textparse_token_item_t *item = textparse_get_first_token(handle);
    while (item)
    {
        print_textparse_token_item(handle, item, 0);

        item = item->next;
    }

cleanup:
    if(handle)
    {
        textparse_close(handle);
        handle = NULL;
    }

    return ret;
}
