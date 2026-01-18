#include <textparser.h>
#include <cfml_definition.json.h>
#include <json_definition.json.h>
//#include <php_definition.json.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


const textparser_language_definition *definitions[] = {&cfml_definition, &json_definition, nullptr};

static const textparser_language_definition *get_language_definition_by_filename(const char *filename)
{
    const textparser_language_definition *definition = nullptr;
    const char *definition_ext = nullptr;
    int def_cnt = 0;

    const char *filename_ext = strrchr(filename, '.');
    if (filename_ext == nullptr)
        return nullptr;

    filename_ext++;

    while(definitions[def_cnt] != nullptr)
    {
        int ext_cnt = 0;

        definition = definitions[def_cnt];

        while(definition->default_file_extensions[ext_cnt] != nullptr)
        {
            definition_ext = definition->default_file_extensions[ext_cnt];

            if (strcmp(definition_ext, filename_ext) == 0)
                return definition;

            ext_cnt++;
        }

        def_cnt++;
    }

    return nullptr;
}

static void print_textparser_token_item(void *handle, textparser_token_item *item, int level, bool colored)
{
    //const textparser_language_definition *language;
    const char *token_name = nullptr;
    char *token_text = nullptr;

    for(int c = 0; c < level; c++)
        putc(' ', stdout);

    token_name = textparser_get_token_id_name(handle, item->token_id);
    token_text = textparser_get_token_text(handle, item);

    if (colored)
    {
        if ((token_text)&&((item->child == nullptr)||(strlen(token_text) < 50))) {
            printf("type: \033[48;5;4m%s\033[0m, position: %zd, length: %zd, text: \033[48;5;5m%s\033[0m\n", token_name, item->position, item->len, token_text);
        } else {
            printf("type: \033[48;5;4m%s\033[0m, position: %zd, length: %zd\n", token_name, item->position, item->len);
        }
    }
    else
    {
        if ((token_text)&&((item->child == nullptr)||(strlen(token_text) < 50))) {
            printf("type: %s, position: %zd, length: %zd, text: %s\n", token_name, item->position, item->len, token_text);
        } else {
            printf("type: %s, position: %zd, length: %zd\n", token_name, item->position, item->len);
        }
    }

    free(token_text);

    struct textparser_token_item *child = item->child;
    while(child)
    {
        print_textparser_token_item(handle, child, level + 1, colored);
        child = child->next;
    }
}

void usage()
{
    fprintf(stderr, "Usage: textparser <file> [--no-color]\n");
}

int main(int argc, const char *argv[])
{
    bool colored = true;

    const textparser_language_definition *language_def = nullptr;
    textparser_defer(handle);

    if ((argc < 2)||(argc > 3))
    {
        usage();
        return EXIT_FAILURE;
    }

    if (argc == 3)
    {
        if (strcmp(argv[2], "--no-color") == 0)
        {
            colored = false;
        }
        else
        {
            usage();
            return EXIT_FAILURE;
        }
    }

    const char *filename = argv[1];

    language_def = get_language_definition_by_filename(filename);
    if (language_def == nullptr)
    {
        fprintf(stderr, "Unsupported file extension for file %s\n", filename);
        return EXIT_FAILURE;
    }

    int err = textparser_openfile(filename, language_def->default_text_encoding, &handle);
    if (err)
    {
        fprintf(stderr, "textparser_openfile returned with error code: %d\n", err);
        return EXIT_FAILURE;
    }

    err = textparser_parse(handle, language_def);
    if (err)
    {
        fprintf(stderr, "textparser_parse returned with error code: %d\n", err);
        return EXIT_FAILURE;
    }

    printf("File parsed ok!\n");

    for(textparser_token_item *item = textparser_get_first_token(handle); item != nullptr; item = item->next)
    {
        print_textparser_token_item(handle, item, 0, colored);
    }

    return EXIT_SUCCESS;
}
