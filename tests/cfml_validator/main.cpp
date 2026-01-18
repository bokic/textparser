#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

#include <textparser.h>
#include "../../definitions/cfml_definition.json.h"


static int parseFile(const QString &filename)
{
    QString itemFileName = QDir::toNativeSeparators(filename);

    textparser_defer(handle);

    int res = textparser_openfile(itemFileName.toUtf8().constData(), TEXTPARSER_ENCODING_LATIN1, &handle);
    if (res)
    {
        fprintf(stderr, "textparser_openfile() failed to open file(%s). Error code: %d\n", filename.toUtf8().constData(), res);
        return EXIT_FAILURE;
    }

    printf("Parsing. file(%s).\n", filename.toUtf8().constData()); fflush(stdout);

    res = textparser_parse(handle, &cfml_definition);
    if (res)
    {
        fprintf(stderr, "textparser_parse() failed to parse file(%s). Error: %s\n", filename.toUtf8().constData(), textparser_parse_error(handle));
        return EXIT_FAILURE;
    }

    printf("ok. file(%s).\n", filename.toUtf8().constData()); fflush(stdout);

    return EXIT_SUCCESS;
}

static int parseDir(const QString &path)
{
    QDir dir(path);

    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::DirsFirst | QDir::Name);
    QStringList filters;
    filters << "*.cfm" << "*.cfc";
    dir.setNameFilters(filters);
    QFileInfoList files = dir.entryInfoList();

    for (int c = 0; c < files.size(); c++)
    {
        QFileInfo fileinfo = files.at(c);
        int res = 0;

        if (fileinfo.isDir() == true)
        {
            res = parseDir(fileinfo.absoluteFilePath());
            if (res != EXIT_SUCCESS)
            {
                return res;
            }
        }
        else
        {
            res = parseFile(fileinfo.absoluteFilePath());
            if (res != EXIT_SUCCESS)
            {
                return res;
            }
        }
    }

    return EXIT_SUCCESS;
}

static void usage()
{
    printf("Usage cfml_validator <dir|cfml_file>\n");
}

int main(int argc, const char *argv[])
{
    const char *dir_file = nullptr;

    if (argc != 2)
    {
        usage();
        return EXIT_SUCCESS;
    }

    dir_file = argv[1];

    if (QFileInfo(dir_file).isFile())
    {
        return parseFile(dir_file);
    }
    else
    {
        return parseDir(dir_file);
    }
}
