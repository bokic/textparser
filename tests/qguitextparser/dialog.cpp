#include "dialog.h"
#include "ui_dialog.h"

#include <QSettings>

#include "textparser.h"
#include <json_definition.json.h>
#include <cfml_definition.json.h>


static void reformatWidget(const language_definition *definition, QList<QCodeEditWidgetLine> &lines)
{
    textparser_parser_state_defer(state);
    textparser_defer(handle);
    QByteArray textBA;
    QString text;
    int res = 0;

    for(const auto &line: lines)
    {
        text.append(line.text);

        switch(line.type)
        {
        case QTextParserLine::QTextParserLineTypeCREndLine:
            text.append('\r');
            break;
        case QTextParserLine::QTextParserLineTypeLFEndLine:
            text.append('\n');
            break;
        case QTextParserLine::QTextParserLineTypeCRLFEndLine:
            text.append("\r\n");
            break;
        default:
            break;
        }
    }

    textBA = text.toLatin1();

    res = textparser_openmem(textBA.constData(), textBA.length(), ADV_REGEX_TEXT_LATIN1, &handle);
    if (res != 0)
    {
        qDebug("textparser_openmem() FAILED!");
        return;
    }

    res = textparser_parse(handle, definition);
    if (res != 0)
    {
        qDebug() << "textparser_parse() FAILED. Error: " << textparser_parse_error(handle);
        return;
    }

    state = textparser_state_new(handle);

    int offset = 0;
    for(auto &line: lines)
    {
        line.colors.clear();

        auto len = line.text.length();
        for(int c = 0; c < len; c++)
        {
            auto chType = state->state[offset + c];
            if (chType != 0)
            {
                QColor foregroundColor;
                QColor backgroundColor;

                if (definition->tokens[chType].text_color      != TEXTPARSER_NOCOLOR) foregroundColor = QColor((definition->tokens[chType].text_color      >> 0) & 0xff, (definition->tokens[chType].text_color      >> 8) & 0xff, (definition->tokens[chType].text_color      >> 16) & 0xff);
                if (definition->tokens[chType].text_background != TEXTPARSER_NOCOLOR) backgroundColor = QColor((definition->tokens[chType].text_background >> 0) & 0xff, (definition->tokens[chType].text_background >> 8) & 0xff, (definition->tokens[chType].text_background >> 16) & 0xff);

                if ((foregroundColor.isValid())||(backgroundColor.isValid()))
                {
                    QTextParserColorItem col(c, 1, foregroundColor, backgroundColor);

                    line.colors.append(col);
                }
            }
        }

        offset += line.text.length();

        switch(line.type)
        {
        case QTextParserLine::QTextParserLineTypeCREndLine:
        case QTextParserLine::QTextParserLineTypeLFEndLine:
            offset += 1;
            break;
        case QTextParserLine::QTextParserLineTypeCRLFEndLine:
            offset += 2;
            break;
        default:
            break;
        }
    }
}

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    ui->verticalSplitter->setSizes({200, 200});
    ui->horizontalSplitter->setSizes({200, 200});

    ui->verticalSplitter->setStretchFactor(0, 0);
    ui->verticalSplitter->setStretchFactor(1, 1);
    ui->horizontalSplitter->setStretchFactor(1, 0);
    ui->horizontalSplitter->setStretchFactor(0, 1);

    loadState();
}

Dialog::~Dialog()
{   
    delete ui;
    ui = nullptr;
}

void Dialog::on_plainTextEdit_parserUpdate()
{
    qDebug() << "void Dialog::on_plainTextEdit_parserUpdate()";

    auto lines = ui->plainTextEdit->lines();

    reformatWidget(&cfml_definition, lines);

    ui->plainTextEdit->setLines(lines);
}

void Dialog::on_parserDefinitionWidget_parserUpdate()
{
    qDebug() << "void Dialog::on_parserDefinitionWidget_parserUpdate()";

    auto lines = ui->parserDefinitionWidget->lines();

    reformatWidget(&json_definition, lines);

    ui->parserDefinitionWidget->setLines(lines);
}

void Dialog::on_plainTextEdit_textChanged()
{
    qDebug() << "void Dialog::on_plainTextEdit_textChanged()";

    saveState();
}

void Dialog::on_parserDefinitionWidget_textChanged()
{
    qDebug() << "void Dialog::on_parserDefinitionWidget_textChanged()";

    saveState();
}

void Dialog::loadState()
{
    QSettings settings;

    auto parserDefinitionWidget_text = settings.value("parserDefinitionWidget_text").toString();
    auto plainTextEdit_text = settings.value("plainTextEdit_text").toString();

    ui->parserDefinitionWidget->setText(parserDefinitionWidget_text);
    ui->plainTextEdit->setText(plainTextEdit_text);
}

void Dialog::saveState()
{
    QSettings settings;

    auto plainTextEdit_text = ui->plainTextEdit->text();
    auto parserDefinitionWidget_text = ui->parserDefinitionWidget->text();

    settings.setValue("plainTextEdit_text", plainTextEdit_text);
    settings.setValue("parserDefinitionWidget_text", parserDefinitionWidget_text);
}
