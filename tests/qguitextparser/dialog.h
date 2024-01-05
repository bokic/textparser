#pragma once

#include "qjsonhighlighter.h"

#include <QDialog>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog final : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

protected:
    virtual void timerEvent(QTimerEvent *e) override;

private slots:
    void on_plainTextEdit_textChanged();
    void on_parserDefinitionWidget_textChanged();

private:
    void startTextChangeTimer();
    void updateState();

    Ui::Dialog *ui = nullptr;

    QJsonHighlighter jsonHighlighter;
    int m_Timer = 0;
};
