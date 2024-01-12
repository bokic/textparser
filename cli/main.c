#include <textparser.h>
//#include <cfml_definition.h>
#include <json_definition.h>
//#include <php_definition.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


_Static_assert(__STDC_VERSION__ == 202000, "Wrong C standard");

static void print_textparser_token_item(void *handle, textparser_token_item *item, int level)
{
    const char *token_name = NULL;
    char *token_text = NULL;

    for(int c = 0; c < level; c++)
        putc(' ', stdout);

    token_name = textparser_get_token_id_name(handle, item->token_id);
    token_text = textparser_get_token_text(handle, item);

    if ((token_text)&&((item->child == NULL)||(strlen(token_text) < 50))) {
        printf("type: \\e[48;5;4m%s\\e[0m, text: \\e[48;5;5m%s\\e[0m\n", token_name, token_text);
    } else {
        printf("type: \\e[48;5;4m%s\\e[0m\n", token_name);
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
        printf("Usage: textparser <file.json>\n");

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

    err = textparser_parse(handle, &json_definition);
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
