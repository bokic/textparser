#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_textEdit_marginClicked(int margin, int line, const Qt::KeyboardModifiers &state);

    void on_textEdit_SCN_ZOOM();

    void on_textEdit_linesChanged();

    void updatePanelWidth();

private:
    Ui::MainWindow *ui;
    int m_breakpointMarker = 0;
    int m_breakpointPendingMarker = 0;
    int m_breakpointDisabledMarker = 0;
};
