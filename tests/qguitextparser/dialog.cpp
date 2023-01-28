#include "dialog.h"
#include "./ui_dialog.h"


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint| Qt::WindowCloseButtonHint);

    ui->splitter->setSizes({200, 1});
    ui->splitter_2->setSizes({1, 200});
}

Dialog::~Dialog()
{
    delete ui;
}
