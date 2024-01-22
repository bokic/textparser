#include "qjsonhighlighter.h"
#include <textparser.h>

#include <json_definition.json.h>


QJsonHighlighter::QJsonHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
    init();
}

void QJsonHighlighter::highlightBlock(const QString &text)
{
    QByteArray textBA;
    void *handle = nullptr;
    int res = 0;

    textBA = text.toLatin1();

    qDebug() << "previousBlockState(): " << previousBlockState() << ", currentBlockState(): " << currentBlockState() << ", Text: " << text;

    res = textparser_openmem(textBA.constData(), textBA.length(), ADV_REGEX_TEXT_LATIN1, &handle);
    if ((res == 0)&&(handle))
    {
        res = textparser_parse(handle, &json_definition);
        if (res != 0) {
            qDebug() << "Parser error!";
            goto cleanup;
        }

        textparser_dump(handle);

        textparser_token_item *token = textparser_get_first_token(handle);
        if (token)
            updateToken(token);

        qDebug() << "Parser ok";
    }
    else
    {
        qDebug() << "textparser_openmem FAILED";
    }

cleanup:
    textparser_close(handle);
}

void QJsonHighlighter::init()
{
    QTextCharFormat fmt;


    //TextParser_json_Object,
    fmt = QTextCharFormat();
    //fmt.setFontWeight(QFont::Bold);
    //fmt.setForeground(Qt::darkCyan);
    m_formats << fmt;

    //TextParser_json_Array,
    fmt = QTextCharFormat();
    //fmt.setFontWeight(QFont::Bold);
    //fmt.setForeground(Qt::darkCyan);
    m_formats << fmt;

    //TextParser_json_String,
    fmt = QTextCharFormat();
    //fmt.setFontWeight(QFont::Bold);
    fmt.setForeground(QColor(170, 17, 17, 255));
    m_formats << fmt;

    //TextParser_json_Number,
    fmt = QTextCharFormat();
    //fmt.setFontWeight(QFont::Bold);
    fmt.setForeground(QColor(17, 102, 68, 255));
    m_formats << fmt;

    //TextParser_json_Operator,
    fmt = QTextCharFormat();
    m_formats << fmt;

    //TextParser_json_Boolean,
    fmt = QTextCharFormat();
    fmt.setFontWeight(QFont::Bold);
    m_formats << fmt;

    //TextParser_json_KeySeparator,
    fmt = QTextCharFormat();
    m_formats << fmt;

    //TextParser_json_ValueSeparator,
    fmt = QTextCharFormat();
    m_formats << fmt;

    // TextParser_json_EscapeCharacters
    fmt = QTextCharFormat();
    fmt.setForeground(QColor(255, 32, 32, 255));
    m_formats << fmt;
}

void QJsonHighlighter::updateToken(textparser_token_item *token)
{
    if ((token->token_id >= 0)&&(token->token_id < m_formats.length()))
    {
        setCurrentBlockState(token->token_id);
        setFormat(token->position, token->len, m_formats[token->token_id]);
    }

    if (token->child)
        updateToken(token->child);

    if (token->next)
        updateToken(token->next);
}
