#pragma once

//#include <qtextparser.h>

#include <qtwidgetsglobal.h>
#include <QAbstractScrollArea>
#include <QPixmap>
#include <QString>
#include <QColor>
#include <QFont>
#include <QList>
#include <QPen>


class QCodeEditWidgetLine;

class QCodeEditWidgetTextPosition final
{
public:
    int m_Row = 1;
    int m_Column = 1;
};

class QTextParserColorItem final
{
public:
    int index = -1;
    int length = -1;
    QColor foregroundColor;
};

class QCodeEditWidget final: public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(QString Text READ text WRITE setText DESIGNABLE false)
    Q_PROPERTY(int TabSize READ tabSize WRITE setTabSize DESIGNABLE true)

public:
    enum QLineStatusType {QLineStatusTypeLineNotModified, QLineStatusTypeLineSaved, QLineStatusTypeLineModified};
    enum QBreakpointType {QBreakpointTypeNoBreakpoint, QBreakpointTypeBreakpoint, QBreakpointTypeBreakpointPending, QBreakpointTypeDisabled};

    explicit QCodeEditWidget(QWidget *parent = nullptr);
    virtual ~QCodeEditWidget() override;

    QString text() const;
    int tabSize() const;
    void setFileExtension(const QString &extension);
    void clearFormatting();
    void addFormat(int p_line, const QTextParserColorItem &p_item);
    void setBreakpoint(int line, QBreakpointType type);
    QBreakpointType breakpoint(int line) const;

public Q_SLOTS:
    void setText(QString text);
    void setTabSize(int size);

Q_SIGNALS:
    void key_press(QKeyEvent *event);
    void textChanged();
    void breakpoint_change(int line);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updatePanelWidth();
    void ensureCaretIsVisible();

    QBrush m_LineNumbersBackground;
    QPen m_LineNumbersNormal;
    QPen m_LineNumbersCurrent;
    QBrush m_LineModifiedAndNotSavedBackground;
    QBrush m_LineModifiedAndSavedBackground;
    QBrush m_CurrentLineBackground;
    QFont m_TextFont;
    QPixmap m_BreakPointPixmap;
    QPixmap m_BreakPointPixmapPending;
    QPixmap m_BreakPointPixmapDisabled;
    QList<QCodeEditWidgetLine> m_Lines;
//    QTextParser m_Parser;

    int m_ScrollXCharPos = 0;
    int m_ScrollYLinePos = 0;
    int m_LineNumbersPanelWidth = 3;
    int m_currentlyBlinkCursorShowen = 0;
    int m_CursorTimerID = 0;
    int m_FontHeight = 0;
    int m_LineHeight = 0;
    int m_LineYOffset = 0;
    bool m_SelectMouseDown = false;
    int m_tabSize = 4;

    QCodeEditWidgetTextPosition m_CarretPosition;
    QCodeEditWidgetTextPosition m_SelectionPosition;
};

class QCodeEditWidgetLine final
{
public:
    QString text;
    int type;
    QCodeEditWidget::QBreakpointType breakpointType = QCodeEditWidget::QBreakpointTypeNoBreakpoint;
    QCodeEditWidget::QLineStatusType lineStatus = QCodeEditWidget::QLineStatusTypeLineNotModified;
    QList<QTextParserColorItem> colors;
};
