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

    /*char *tt = NULL;
    tt = "e";
    tt = cfml_definition.default_file_extensions[0];
    tt = cfml_definition.default_file_extensions[1];
    tt = cfml_definition.default_file_extensions[2];*/

    if (argc != 2)
    {
        printf("Usage: textparser <file.cfm|cfc>\n");

        return EXIT_FAILURE;
    }

    err = textparse_openfile(argv[1], TEXTPARSE_LATIN_1, &handle);
    if (err)
    {
        fprintf(stderr, "textparse_openfile returned with error code: %d\n", err);

        return EXIT_FAILURE;
    }

    //memset_explicit(0,0);

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
