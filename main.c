#include <textparser.h>
#include <cfml_definition.h>
#include <stddef.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
    void *handle = NULL;

    char *tt = NULL;

    tt = "e";

    tt = cfml_definition.default_file_extensions[0];
    tt = cfml_definition.default_file_extensions[1];
    tt = cfml_definition.default_file_extensions[2];

    if (argc != 2)
    {
        printf("Usage: textparser <file.cfm|cfc>\n");

        return 1;
    }

    textparse_parsefile(argv[1], &handle);

    if(handle)
    {
        textparse_close(handle);
        handle = NULL;
    }

    return 0;
}
