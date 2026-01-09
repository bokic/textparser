#pragma once

#include <QAbstractScrollArea>
#include <QPixmap>
#include <QString>
#include <QColor>
#include <QFont>
#include <QList>
#include <QPen>

#ifndef QCEW_STATIC
#ifdef QCEW_SHARED_EXPORT
#define QCEW_EXPORT Q_DECL_EXPORT
#else
#define QCEW_EXPORT Q_DECL_IMPORT
#endif
#else
#define QCEW_EXPORT
#endif

class QCEW_EXPORT QCodeEditWidgetLine;

class QCEW_EXPORT QTextParserLine
{
public:
    enum QTextParserLineType
    {
        QTextParserLineTypeNoEndLine,
        QTextParserLineTypeCREndLine,
        QTextParserLineTypeLFEndLine,
        QTextParserLineTypeCRLFEndLine,
    };

    QTextParserLine() = default;
    ~QTextParserLine() = default;
    QTextParserLine(const QTextParserLine &other) = default;

    inline QTextParserLine &operator=(QTextParserLine &&other) = default;
    inline QTextParserLine &operator=(const QTextParserLine &other) = default;

    QString text;
    QTextParserLineType type = QTextParserLineTypeNoEndLine;
};

typedef QList<QTextParserLine> QTextParserLines;

class QCEW_EXPORT QCodeEditWidgetTextPosition final
{
public:
    qsizetype m_Row = 1;
    qsizetype m_Column = 1;
};

inline bool operator==(const QCodeEditWidgetTextPosition& a, const QCodeEditWidgetTextPosition& b)
{
    return ((a.m_Row == b.m_Row)&&(a.m_Column == b.m_Column));
}

class QCEW_EXPORT QTextParserColorItem final
{
public:
    QTextParserColorItem(int index, int length, QColor foregroundColor, QColor backgroundColor)
        : index(index)
        , length(length)
        , foregroundColor(foregroundColor)
        , backgroundColor(backgroundColor)
    {
    };

    int index = -1;
    int length = -1;
    QColor foregroundColor;
    QColor backgroundColor;
};

class QCEW_EXPORT QCodeEditWidget final: public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(QString Text READ text WRITE setText DESIGNABLE false CONSTANT)
    Q_PROPERTY(int TabSize READ tabSize WRITE setTabSize NOTIFY tabSizeChanged)

public:
    enum QLineStatusType {QLineStatusTypeLineNotModified, QLineStatusTypeLineSaved, QLineStatusTypeLineModified};
    enum QBreakpointType {QBreakpointTypeNoBreakpoint, QBreakpointTypeBreakpoint, QBreakpointTypeBreakpointPending, QBreakpointTypeDisabled};

    explicit QCodeEditWidget(QWidget *parent = nullptr);
    virtual ~QCodeEditWidget() override;

    QString text() const;
    int tabSize() const;
    void setFileExtension(const QString &extension);
    void clearFormatting();
    void addFormat(int line, const QTextParserColorItem &item);
    void setFormatFromJavaParser(const QByteArray format);
    void setBreakpoint(int line, QBreakpointType type);
    void setDebuggerAtLine(int line);
    QBreakpointType breakpoint(int line) const;
    bool canUndo() const;
    bool canRedo() const;
    bool hasSelection() const;
    void setLines(const QList<QCodeEditWidgetLine> &lines);
    QList<QCodeEditWidgetLine> lines() const;

public slots:
    void setText(QString text);
    void setTabSize(int size);

Q_SIGNALS:
    void key_press(QKeyEvent *event);
    void parserUpdate();
    void textChanged();
    void breakpoint_change(int line);
    void tabSizeChanged(int tabSize);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void scrollContentsBy(int dx , int dy) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void updatePanelWidth();
    void ensureCaretIsVisible();
    void deleteSelectedBlock();

    QPen m_LineNumbersNormal;
    QPen m_LineNumbersCurrent;
    QBrush m_LineModifiedAndNotSavedBackground;
    QBrush m_LineModifiedAndSavedBackground;
    QBrush m_CurrentLineBackground;
    int m_FontSize = 10;
    QFont m_TextFont;
    QFont m_TextFontBold;
    QPixmap m_BreakPointPixmap;
    QPixmap m_BreakPointPixmapPending;
    QPixmap m_BreakPointPixmapDisabled;
    QList<QCodeEditWidgetLine> m_Lines;

    int m_ScrollXCharPos = 0;
    int m_ScrollYLinePos = 0;
    int m_LineNumbersPanelWidth = 3;
    int m_currentlyBlinkCursorShowen = 1;
    int m_CursorTimerID = 0;
    int m_FontHeight = 0;
    int m_LineHeight = 0;
    int m_LineYOffset = 0;
    bool m_SelectMouseDown = false;
    int m_debuggerAtLine = -1;
    int m_tabSize = 4;

    QCodeEditWidgetTextPosition m_CarretPosition;
    QCodeEditWidgetTextPosition m_SelectionPosition;
};

class QCEW_EXPORT QCodeEditWidgetLine final: public QTextParserLine
{
public:
    QCodeEditWidget::QBreakpointType breakpointType = QCodeEditWidget::QBreakpointTypeNoBreakpoint;
    QCodeEditWidget::QLineStatusType lineStatus = QCodeEditWidget::QLineStatusTypeLineNotModified;
    QList<QTextParserColorItem> colors;
};
