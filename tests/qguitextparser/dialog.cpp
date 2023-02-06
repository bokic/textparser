#include "dialog.h"
#include "./ui_dialog.h"


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

    jsonHighlighter.setDocument(ui->parserDefinitionWidget->document());
}

Dialog::~Dialog()
{
    delete ui;
    ui = nullptr;
}
