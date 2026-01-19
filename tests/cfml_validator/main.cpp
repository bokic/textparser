#include <filesystem>
#include <string>
#include <print>

#include <textparser.h>
#include <cfml_definition.json.h>


using namespace std::filesystem;
using namespace std;

static int parseFile(const path &filename)
{
    string itemFileName = filename.string();

    textparser_defer(handle);

    int res = textparser_openfile(itemFileName.c_str(), TEXTPARSER_ENCODING_LATIN1, &handle);
    if (res)
    {
        println(stderr, "textparser_openfile() failed to open file({}). Error code: {}", itemFileName, res);
        return EXIT_FAILURE;
    }

    println("Parsing. file({}).", itemFileName);

    res = textparser_parse(handle, &cfml_definition);
    if (res)
    {
        println(stderr, "textparser_parse() failed to parse file({}). Error: {}", itemFileName, textparser_parse_error(handle));
        return EXIT_FAILURE;
    }

    println("ok. file({}).", itemFileName);

    return EXIT_SUCCESS;
}

static int parseDir(const path &path)
{
    for (const auto& entry : recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            string ext = entry.path().extension().string();
            if (ext == ".cfm" || ext == ".cfc")
            {
                int res = parseFile(entry.path());
                if (res != EXIT_SUCCESS)
                {
                    return res;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

static void usage()
{
    println("Usage cfml_validator <dir|cfml_file>");
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        usage();
        return EXIT_SUCCESS;
    }

    path dir_file(argv[1]);

    if (is_regular_file(dir_file))
    {
        return parseFile(dir_file);
    }
    else if (is_directory(dir_file))
    {
        return parseDir(dir_file);
    }
    else
    {
        println(stderr, "Invalid path: {}", dir_file.string());
    }

    return EXIT_FAILURE;
}
