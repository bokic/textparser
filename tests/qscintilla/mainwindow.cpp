#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercustom.h>
#include <Qsci/qsciabstractapis.h>
#include <Qsci/qscistyle.h>

#include <textparser.h>
#include <../definitions/cfml_definition.json.h>


class QsciLexerTextParser: public QsciLexerCustom
{
public:
    QsciLexerTextParser(const textparser_language_definition *definition)
        : m_definition(definition)
    {
        if (m_definition)
        {
            for(int c = 0;;c++)
            {
                if (m_definition->tokens[c].name == nullptr)
                {
                    m_tokenCnt = c;
                    break;
                }

                if (m_definition->tokens[c].text_color != -1)
                    setColor(QColor::fromRgb(m_definition->tokens[c].text_color >> 16 & 0xff, m_definition->tokens[c].text_color >> 8 & 0xff, m_definition->tokens[c].text_color >> 0 & 0xff), c + 1);
                if (m_definition->tokens[c].text_background != -1)
                    setColor(QColor::fromRgb(m_definition->tokens[c].text_background >> 16 & 0xff, m_definition->tokens[c].text_background >> 8 & 0xff, m_definition->tokens[c].text_background >> 0 & 0xff), c + 1);
            }
        }
    }

    const char *language() const override
    {
        if (m_definition == nullptr)
            return "{not set}";

        return m_definition->name;
    }

    QString description(int style) const override
    {
        QString ret;

        if (style < m_tokenCnt)
            ret = m_definition->tokens[style].name;

        return ret;
    }

    void recursivelyStyleText(const textparser_token_item *token)
    {
        if (token == nullptr)
            return;

        startStyling(token->position);
        setStyling(token->len, token->token_id + 1);

        if (token->child)
        {
            recursivelyStyleText(token->child);
        }

        if (token->next)
        {
            recursivelyStyleText(token->next);
        }
    }

    void styleText(int start, int end) override
    {
        auto text = editor()->text().toLatin1();
        if (text.length() <= 0)
            return;

        textparser_defer(parser);
        textparser_openmem(text.constData(), text.length(), TEXTPARSER_ENCODING_LATIN1, &parser);

        if (textparser_parse(parser, m_definition) != 0)
            return;

        startStyling(0);
        setStyling(text.length(), 0);

        const textparser_token_item *token = textparser_get_first_token(parser);

        recursivelyStyleText(token);
    }

private:
    const textparser_language_definition *m_definition = nullptr;
    int m_tokenCnt = 0;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->textEdit->setLexer(new QsciLexerTextParser(&cfml_definition));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_textEdit_textChanged()
{
    QString text = ui->textEdit->text();
}
