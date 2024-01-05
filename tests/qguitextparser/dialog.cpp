#include "dialog.h"
#include "ui_dialog.h"
#include <QSettings>


Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    QSettings settings;

    ui->setupUi(this);

    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

    ui->verticalSplitter->setSizes({200, 200});
    ui->horizontalSplitter->setSizes({200, 200});

    ui->verticalSplitter->setStretchFactor(0, 0);
    ui->verticalSplitter->setStretchFactor(1, 1);
    ui->horizontalSplitter->setStretchFactor(1, 0);
    ui->horizontalSplitter->setStretchFactor(0, 1);

    jsonHighlighter.setDocument(ui->parserDefinitionWidget->document());

    ui->parserDefinitionWidget->setPlainText(settings.value("parserDefinitionWidget_text").toString());
    ui->plainTextEdit->setPlainText(settings.value("plainTextEdit_text").toString());
}

Dialog::~Dialog()
{   
    updateState();

    delete ui;
    ui = nullptr;
}

void Dialog::timerEvent(QTimerEvent *e)
{
    updateState();

    killTimer(m_Timer);
    m_Timer = 0;
}

void Dialog::on_parserDefinitionWidget_textChanged()
{
    startTextChangeTimer();
}

void Dialog::on_plainTextEdit_textChanged()
{
    startTextChangeTimer();
}

void Dialog::startTextChangeTimer()
{
    if (m_Timer)
        killTimer(m_Timer);

    m_Timer = startTimer(5000);
}

void Dialog::updateState()
{
    QSettings settings;

    qDebug() << "void Dialog::updateState()";

    settings.setValue("plainTextEdit_text", ui->plainTextEdit->toPlainText());
    settings.setValue("parserDefinitionWidget_text", ui->parserDefinitionWidget->toPlainText());
}
