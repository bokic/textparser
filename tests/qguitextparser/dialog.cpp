#include "dialog.h"
#include "./ui_dialog.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    ui->verticalSplitter->setSizes({200, 1});
    ui->horizontalSplitter->setSizes({1, 200});

    //connect(ui->parserDefinitionWidget, &QCodeEditWidget::textChanged, this, &Dialog::on_parserDefinitionWidget_textchanged);

    jsonHighlighter.setDocument(ui->parserDefinitionWidget->document());
}

Dialog::~Dialog()
{
    delete ui;
}

/*void Dialog::on_parserDefinitionWidget_textChanged()
{
    QByteArray textBA;
    QString textStr;
    void *handle = NULL;
    QString style;
    int res = 0;

    textStr = ui->parserDefinitionWidget->toPlainText();
    textBA = textStr.toLatin1();

    res = textparse_openmem(textBA.constData(), textBA.length(), TEXTPARSE_LATIN_1, &handle);
    if (res == 0)
    {
        //ui->parserDefinitionWidget.
        res = textparse_parse(handle, &json_definition);
        if (res != 0)
            style = "background: red";
    }
    else
    {
        style = "background: yellow";
    }

    ui->parserDefinitionWidget->setStyleSheet(style);

    textparse_close(handle);
}
*/
