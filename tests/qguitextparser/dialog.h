#pragma once

#include <QDialog>
#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Dialog final : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_plainTextEdit_parserUpdate();
    void on_parserDefinitionWidget_parserUpdate();
    void on_plainTextEdit_textChanged();
    void on_parserDefinitionWidget_textChanged();

private:
    void loadState();
    void saveState();

    Ui::Dialog *ui = nullptr;
};
