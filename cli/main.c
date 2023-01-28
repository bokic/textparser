#include <textparser.h>
#include <cfml_definition.h>
#include <json_definition.h>
#include <php_definition.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


_Static_assert(__STDC_VERSION__ == 202000, "Wrong C standard");

int main(int argc, char *argv[])
{
    void *handle = NULL;
    int err = 0;

    if (argc != 2)
    {
        printf("Usage: textparser <file.json>\n");

        return EXIT_FAILURE;
    }

    err = textparse_openfile(argv[1], TEXTPARSE_LATIN_1, &handle);
    if (err)
    {
        fprintf(stderr, "textparse_openfile returned with error code: %d\n", err);

        return EXIT_FAILURE;
    }

    err = textparse_parse(handle, &json_definition);
    if (err)
    {
        fprintf(stderr, "textparse_parse returned with error code: %d\n", err);

        return EXIT_FAILURE;
    }

    printf("File parsed ok!\n");

    if(handle)
    {
        textparse_close(handle);
        handle = NULL;
    }

    return EXIT_SUCCESS;
}
