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

    jsonHighlighter.setDocument(ui->parserDefinitionWidget->document());
}

Dialog::~Dialog()
{
    delete ui;
    ui = nullptr;
}
