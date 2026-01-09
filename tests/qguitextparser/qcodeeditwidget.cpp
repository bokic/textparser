#include "qcodeeditwidget.h"

#include <algorithm>

#include <QPainter>
#include <QApplication>
#include <QFontMetrics>
#include <QTimerEvent>
#include <QScrollBar>
#include <QClipboard>
#include <QKeyEvent>
#include <QEvent>
#include <QImage>
#include <QColor>

#include <math.h>


QCodeEditWidget::QCodeEditWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
    , m_LineNumbersNormal(QPen(QColor(172, 168, 153)))
    , m_LineNumbersCurrent(QPen(QColor(128, 128, 128)))
    , m_LineModifiedAndNotSavedBackground(QColor(128, 0, 0))
    , m_LineModifiedAndSavedBackground(QColor(0, 128, 0))
    , m_CurrentLineBackground(QColor(224, 233, 247))
#ifdef Q_OS_WIN
    , m_TextFont(QFont("Courier", m_FontSize, QFont::Normal))
#elif defined Q_OS_LINUX
    , m_TextFont(QFont("Droid Sans Mono", m_FontSize, QFont::Normal))
#elif defined Q_OS_MACOS
    , m_TextFont(QFont("Monaco", m_FontSize, QFont::Normal))
#else
#error Unsupported OS.
#endif
{
    QFontMetrics l_fm = QFontMetrics(m_TextFont);
    setAutoFillBackground(false);

    m_TextFont.setStyleHint(QFont::Courier, QFont::PreferAntialias);

    m_FontHeight = l_fm.height();
    m_LineHeight = m_FontHeight + 1;

    m_LineYOffset = m_LineHeight - l_fm.descent() - 1;

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setText("");

    setFocusPolicy(Qt::WheelFocus);

    m_CursorTimerID = startTimer(500);
}

QCodeEditWidget::~QCodeEditWidget()
{
    killTimer(m_CursorTimerID);
}

void QCodeEditWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() ==  m_CursorTimerID)
    {
        m_currentlyBlinkCursorShowen ^= 1;

        //viewport()->update(0, 0, ceil(m_CursorHeight * 0.05) + 1, m_CursorHeight + 1);
        viewport()->update(); // TODO: optimize this call, by updating the affected region only!!
    }
}

void QCodeEditWidget::keyPressEvent(QKeyEvent *event)
{
    bool l_TextUpdated = false;
    auto modifiers = event->modifiers();

    if (modifiers & Qt::ControlModifier)
    {
        return;
    }

    if ((modifiers == Qt::ControlModifier)&&(event->text() == "0"))
    {
        m_FontSize = 10;

        m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
        viewport()->update();

        return;
    }

    switch(event->key())
    {
    case Qt::Key_Home:
        if ((modifiers == Qt::NoModifier)||(modifiers == Qt::ShiftModifier))
        {
            m_CarretPosition.m_Column = 1;
        }
        else if (modifiers == Qt::ControlModifier)
        {
            m_CarretPosition.m_Column = 1;
            m_CarretPosition.m_Row = 1;
        }
        break;

    case Qt::Key_End:
        if ((modifiers == Qt::NoModifier)||(modifiers == Qt::ShiftModifier))
        {
            m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
        }
        else if (modifiers == Qt::ControlModifier)
        {
            m_CarretPosition.m_Row = m_Lines.count();
            m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
        }
        break;

    case Qt::Key_Up:
        if (m_CarretPosition.m_Row <= 1)
        {
            return;
        }

        m_CarretPosition.m_Row--;
        break;

    case Qt::Key_Down:
        if (m_CarretPosition.m_Row < m_Lines.count())
        {
            m_CarretPosition.m_Row++;
        }
        break;

    case Qt::Key_Left:
        if (m_CarretPosition.m_Column <= 1)
        {
            if (m_CarretPosition.m_Row > 1)
            {
                m_CarretPosition.m_Row--;
                m_CarretPosition.m_Column = m_Lines.at(m_CarretPosition.m_Row - 1).text.length() + 1;
            }
            else
            {
                return;
            }
        }
        else
        {
            auto line_size = m_Lines.at(m_CarretPosition.m_Row - 1).text.length();
            if (line_size < m_CarretPosition.m_Column)
                m_CarretPosition.m_Column = std::max(line_size, static_cast<qsizetype>(1));
            else
                m_CarretPosition.m_Column--;
        }
        break;

    case Qt::Key_Right:
        if (m_CarretPosition.m_Column <= m_Lines.at(m_CarretPosition.m_Row - 1).text.length())
        {
            m_CarretPosition.m_Column++;
        }
        else
        {
            if (m_CarretPosition.m_Row < m_Lines.count())
            {
                m_CarretPosition.m_Row++;
                m_CarretPosition.m_Column = 1;
            }
        }
        break;

    case Qt::Key_Return:
        if (modifiers == 0)
        {
            auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
            QString l_NewLineText = line.text.right(line.text.length() - m_CarretPosition.m_Column + 1);
            line.text.remove(m_CarretPosition.m_Column - 1, line.text.length() - m_CarretPosition.m_Column + 1);
            line.lineStatus = QLineStatusTypeLineModified;

            QCodeEditWidgetLine line2;
            line2.text = l_NewLineText;
            line2.type = line.type;
            line2.lineStatus = QLineStatusTypeLineModified;
            line2.breakpointType = QBreakpointTypeNoBreakpoint;

            line.type = QTextParserLine::QTextParserLineTypeLFEndLine;

            m_Lines.insert(m_CarretPosition.m_Row - 1, line);
            m_Lines.insert(m_CarretPosition.m_Row, line2);

            m_CarretPosition.m_Row++;
            m_CarretPosition.m_Column = 1;

            l_TextUpdated = true;
        }
        break;

    case Qt::Key_Backspace:
        if (modifiers == 0)
        {
            if ((m_CarretPosition.m_Row == 1)&&(m_CarretPosition.m_Column == 1))
            {
                break;
            }

            if (m_CarretPosition == m_SelectionPosition)
            {
                if (m_CarretPosition.m_Column > 1)
                {
                    auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
                    auto lineString = line.text;
                    lineString = lineString.left(m_CarretPosition.m_Column - 2) + lineString.right(line.text.length() - m_CarretPosition.m_Column + 1);
                    line.text = lineString;
                    m_Lines.insert(m_CarretPosition.m_Row - 1, line);

                    m_CarretPosition.m_Column--;
                    m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
                }
                else
                {
                    auto line = m_Lines.takeAt(m_CarretPosition.m_Row - 1);
                    auto line2 = m_Lines.takeAt(m_CarretPosition.m_Row - 2);

                    int l_oldPos = line2.text.length();

                    line2.text.append(line.text);

                    m_Lines.insert(m_CarretPosition.m_Row - 2, line2);

                    m_CarretPosition.m_Row--;
                    m_SelectionPosition.m_Row = m_CarretPosition.m_Row;
                    m_CarretPosition.m_Column = l_oldPos + 1;
                    m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
                }

                l_TextUpdated = true;
            }
            else
            {
                deleteSelectedBlock();
            }
        }
        break;

    case Qt::Key_Delete:
        {
            if (m_CarretPosition == m_SelectionPosition)
            {
                auto line = m_Lines.at(m_CarretPosition.m_Row - 1);

                if (m_CarretPosition.m_Column - 1 == line.text.length())
                {
                    if (m_Lines.length() > m_CarretPosition.m_Row)
                    {
                        auto line2 = m_Lines.takeAt(m_CarretPosition.m_Row);

                        line.text.append(line2.text);
                    }
                }
                else
                {
                    line.text.remove(m_CarretPosition.m_Column - 1, 1);
                }
                m_Lines.replace(m_CarretPosition.m_Row - 1, line);

                l_TextUpdated = true;
            }
            else
            {
                deleteSelectedBlock();
            }
        }
        break;

    case Qt::Key_Insert:
    case Qt::Key_C:
        {
            if (modifiers == Qt::ControlModifier)
            {
                QString selectedText = "";

                QClipboard *clipboard = QApplication::clipboard();

                clipboard->setText(selectedText);
            }
        }
        break;
    }

    auto l_EventText = event->text();

    if ((!l_EventText.isEmpty())&&((modifiers & (Qt::ControlModifier | Qt::AltModifier)) == 0))
    {
        deleteSelectedBlock();

        for(const auto &ch: std::as_const(l_EventText))
        {
            if (ch.isPrint())
            {
                auto line = m_Lines.at(m_CarretPosition.m_Row - 1);
                line.text.insert(m_CarretPosition.m_Column - 1, l_EventText);
                line.lineStatus = QLineStatusTypeLineModified;

                m_Lines.replace(m_CarretPosition.m_Row - 1, line);
                m_CarretPosition.m_Column += l_EventText.size();
                m_SelectionPosition.m_Column = m_CarretPosition.m_Column;

                l_TextUpdated = true;
            }
        }
    }

    if ((modifiers & Qt::ShiftModifier) == 0)
    {
        m_SelectionPosition.m_Row = m_CarretPosition.m_Row;
        m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
    }

    if (l_TextUpdated) {
        updatePanelWidth();

        emit textChanged();
        emit parserUpdate();
    } else {
        emit key_press(event);
    }

    m_currentlyBlinkCursorShowen = 1;

    killTimer(m_CursorTimerID);
    m_CursorTimerID = startTimer(500);

    ensureCaretIsVisible();

    viewport()->update(); // TODO: optimize this call, by updating the affected region only!!
}

void QCodeEditWidget::updatePanelWidth()
{
    int l_tmpWidth = 0;

    int l_tmpCount = m_Lines.count();

    while(l_tmpCount > 0)
    {
        l_tmpCount = l_tmpCount / 10;
        l_tmpWidth++;
    }

    if (l_tmpWidth < 2)
    {
        l_tmpWidth = 2;
    }

    m_LineNumbersPanelWidth = l_tmpWidth;
}

void QCodeEditWidget::ensureCaretIsVisible()
{
    if (m_CarretPosition.m_Row <= m_ScrollYLinePos)
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Row - 1);
    }
    else if (m_CarretPosition.m_Row > m_ScrollYLinePos + (viewport()->height() / m_LineHeight))
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Row - (viewport()->height() / m_LineHeight));
    }

    if (m_CarretPosition.m_Column <= m_ScrollXCharPos)
    {
        verticalScrollBar()->setValue(m_CarretPosition.m_Column - 1);
    }
    else if (m_CarretPosition.m_Column > m_ScrollXCharPos + (viewport()->width() / m_LineYOffset))
    {
        verticalScrollBar()->setValue(m_ScrollXCharPos + (viewport()->width() / m_LineYOffset));
    }

    viewport()->update();
}

void QCodeEditWidget::deleteSelectedBlock()
{
    if (m_CarretPosition.m_Row == m_SelectionPosition.m_Row)
    {
        auto row = m_CarretPosition.m_Row - 1;

        if (m_CarretPosition.m_Column > m_SelectionPosition.m_Column)
        {
            auto line = m_Lines[row].text;

            m_Lines[row].text = line.left(m_SelectionPosition.m_Column - 1) + line.right(line.length() - m_CarretPosition.m_Column + 1);

            m_CarretPosition.m_Column = m_SelectionPosition.m_Column;
        }
        else if (m_CarretPosition.m_Column < m_SelectionPosition.m_Column)
        {
            auto line = m_Lines[row].text;

            m_Lines[row].text = line.left(m_CarretPosition.m_Column - 1) + line.right(line.length() - m_SelectionPosition.m_Column + 1);
        }
    }
    else
    {
        // TODO: Implement multiline selection delete block.
    }
}

void QCodeEditWidget::scrollContentsBy(int dx, int dy)
{
    if ((dx == 0)&&(dy == 0))
    {
        return;
    }

    m_ScrollYLinePos -= dy;
    m_ScrollXCharPos -= dx;

    viewport()->update();
}

void QCodeEditWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(viewport());

    bool has_selection;
    int selection_start_row;
    int selection_end_row;

    if ((m_SelectionPosition.m_Row == m_CarretPosition.m_Row)&&(m_SelectionPosition.m_Column == m_CarretPosition.m_Column))
    {
        has_selection = false;

        selection_start_row = 0;
        selection_end_row = 0;
    }
    else
    {
        has_selection = true;

        if (m_SelectionPosition.m_Row > m_CarretPosition.m_Row)
        {
            selection_start_row = m_CarretPosition.m_Row;
            selection_end_row = m_SelectionPosition.m_Row;
        }
        else
        {
            selection_start_row = m_SelectionPosition.m_Row;
            selection_end_row = m_CarretPosition.m_Row;
        }
    }

    QFontMetrics l_fm(m_TextFont);
    int l_fontHeight = l_fm.height();
    int l_fontWidth = l_fm.horizontalAdvance(" ");
    int l_LinesToDraw = (viewport()->height() / l_fontHeight) + 1;
    int l_LineNumbersPanelWidth = 16 + (l_fm.horizontalAdvance(" ") * m_LineNumbersPanelWidth) + 4;
    int l_CharsToDraw = (((viewport()->width() - l_LineNumbersPanelWidth) / l_fontWidth) + 1);

    painter.fillRect(QRect(0, 0, l_LineNumbersPanelWidth, viewport()->height()), palette().dark());

    for(int l_line = 0; l_line < l_LinesToDraw; l_line++)
    {
    	if (m_ScrollYLinePos + l_line >= m_Lines.count())
    	{
    		break;
        }

        if (m_ScrollYLinePos + l_line == m_debuggerAtLine)
        {
            painter.fillRect(l_LineNumbersPanelWidth, l_line * l_fontHeight, viewport()->width(), l_fontHeight, QColorConstants::Red);
        }

        switch(m_Lines.at(m_ScrollYLinePos + l_line).breakpointType)
        {
        case QBreakpointTypeBreakpoint:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmap);
            break;
        case QBreakpointTypeBreakpointPending:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmapPending);
            break;
        case QBreakpointTypeDisabled:
            painter.drawPixmap(2, m_ScrollYLinePos + l_line  * l_fontHeight + 2, m_BreakPointPixmapDisabled);
            break;
        default:
            break;
        }

    	if ((m_ScrollYLinePos + l_line + 1) == m_CarretPosition.m_Row)
    	{
            painter.setFont(m_TextFont);
    		painter.setPen(m_LineNumbersCurrent);
    	}
    	else
    	{
            painter.setFont(m_TextFont);
    		painter.setPen(m_LineNumbersNormal);
    	}

        int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
        QTextOption option;
        option.setTabStopDistance(distance);
        option.setAlignment(Qt::AlignRight);
        painter.drawText(QRect(0, l_line * l_fontHeight, l_LineNumbersPanelWidth - 4, l_fontHeight), QString::number(m_ScrollYLinePos + l_line + 1), option);
        if ((m_ScrollYLinePos + l_line + 1) == m_CarretPosition.m_Row)
    	{
    		painter.setFont(m_TextFont);
        }

        painter.setPen(palette().text().color());

        int l_HorizontalValue = horizontalScrollBar()->value();

        QString l_Line = m_Lines.at(m_ScrollYLinePos + l_line).text;
        const auto &l_LineColors = m_Lines.at(m_ScrollYLinePos + l_line).colors;

        // Paint selection background
        if ((has_selection)&&(m_ScrollYLinePos + l_line >= (selection_start_row - 1))&&(m_ScrollYLinePos + l_line <= (selection_end_row - 1)))
        {
            if (selection_start_row == selection_end_row)
            {
                int from = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                int to = m_CarretPosition.m_Column - 1 - l_HorizontalValue;

                if (from < 0) from = 0;
                if (to < 0) to = 0;

                if (from != to)
                {
                    if (from > to)
                    {
                        int tmp = from;
                        from = to;
                        to = tmp;
                    }

                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else if (m_ScrollYLinePos + l_line == (selection_start_row - 1))
            {
                int from;
                int to;

                if (m_SelectionPosition.m_Row < m_CarretPosition.m_Row)
                {
                    from = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                    to = l_Line.length() - l_HorizontalValue;
                }
                else
                {
                    from = m_CarretPosition.m_Column - 1 - l_HorizontalValue;
                    to = l_Line.length() - l_HorizontalValue;
                }

                if (from < 0) from = 0;
                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else if (m_ScrollYLinePos + l_line == (selection_end_row - 1))
            {
                int from = 0;
                int to = 0;

                if (m_SelectionPosition.m_Row > m_CarretPosition.m_Row)
                {
                    to = m_SelectionPosition.m_Column - 1 - l_HorizontalValue;
                }
                else
                {
                    to = m_CarretPosition.m_Column - 1 - l_HorizontalValue;
                }

                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
            else
            {
                int from = 0;
                int to = l_Line.length() - l_HorizontalValue;

                if (to < 0) to = 0;

                if (to > from)
                {
                    int fromPixel = l_fm.horizontalAdvance(l_Line.left(from));
                    int toPixel = l_fm.horizontalAdvance(l_Line.mid(from, to - from));

                    painter.fillRect(l_LineNumbersPanelWidth + fromPixel, l_line * l_fontHeight, toPixel, l_fontHeight, palette().highlight());
                }
            }
        }

        l_Line = l_Line.left(l_HorizontalValue + l_CharsToDraw);

        //l_Line = l_Line.replace("\t", QString(m_tabSize, ' ')); // TODO: Properly align tabs.

        if (l_HorizontalValue > 0)
        {
            if (l_Line.length() - l_HorizontalValue > 0)
            {
                l_Line = l_Line.right(l_Line.length() - l_HorizontalValue);
            }
            else
            {
                l_Line.clear();
            }
        }

        l_Line = l_Line.left(l_CharsToDraw);

        if (!l_Line.isEmpty())
        {
            QString text = l_Line;
            int current_pos = 0;

            if (!l_LineColors.isEmpty())
            {
                for (auto colorFormat: l_LineColors)
                {
                    if (colorFormat.index > current_pos)
                    {
                        int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(current_pos)) - (m_ScrollXCharPos * l_fontWidth);

                        painter.setPen(palette().text().color());

                        auto textChunk = text.mid(current_pos, colorFormat.index - current_pos);

                        int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                        QTextOption option;
                        option.setTabStopDistance(distance);
                        option.setAlignment(Qt::AlignLeft);
                        painter.drawText(QRect(xPosStart, l_line * l_fontHeight, viewport()->width(), l_fontHeight), textChunk, option);
                        current_pos = colorFormat.index;
                    }

                    if (colorFormat.backgroundColor.isValid())
                    {
                        painter.setPen(Qt::NoPen);
                        painter.setBrush(colorFormat.backgroundColor);

                        int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(colorFormat.index)) - (m_ScrollXCharPos * l_fontWidth);
                        int xPosLen = l_fm.horizontalAdvance(text.mid(colorFormat.index, colorFormat.length)) - (m_ScrollXCharPos * l_fontWidth);

                        painter.drawRect(xPosStart, l_line * l_fontHeight, xPosLen, l_fontHeight);
                    }

                    if (colorFormat.foregroundColor.isValid())
                    {
                        painter.setPen(colorFormat.foregroundColor);
                    }
                    else
                    {
                        painter.setPen(palette().text().color());
                    }

                    int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(colorFormat.index)) - (m_ScrollXCharPos * l_fontWidth);
                    auto textChunk = text.mid(colorFormat.index, colorFormat.length);
                    int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                    QTextOption option;
                    option.setTabStopDistance(distance);
                    option.setAlignment(Qt::AlignLeft);
                    painter.drawText(QRect(xPosStart, l_line * l_fontHeight, viewport()->width(), l_fontHeight), textChunk, option);
                    current_pos = colorFormat.index + colorFormat.length;
                }

                if (current_pos< text.length())
                {
                    painter.setPen(palette().text().color());
                    int xPosStart = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(text.left(current_pos)) - (m_ScrollXCharPos * l_fontWidth);
                    auto textChunk = text.mid(current_pos, text.length() - current_pos);
                    int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                    QTextOption option;
                    option.setTabStopDistance(distance);
                    option.setAlignment(Qt::AlignLeft);
                    painter.drawText(QRect(xPosStart, l_line * l_fontHeight, viewport()->width(), l_fontHeight), textChunk, option);
                }
            }
            else
            {
                painter.setPen(palette().text().color());

                int distance = painter.fontMetrics().horizontalAdvance(" ") * 4;
                QTextOption option;
                option.setTabStopDistance(distance);
                option.setAlignment(Qt::AlignLeft);
                painter.drawText(QRect(l_LineNumbersPanelWidth, l_line * l_fontHeight, viewport()->width(), l_fontHeight), text, option);
            }
        }
    }

    setFont(m_TextFont);

    if ((m_currentlyBlinkCursorShowen == 1)&&((int)m_CarretPosition.m_Column - (int)m_ScrollXCharPos - 1 >= 0))
    {
        int xpos = l_LineNumbersPanelWidth + l_fm.horizontalAdvance(m_Lines.at(m_CarretPosition.m_Row - 1).text.left(m_CarretPosition.m_Column - 1)) - (m_ScrollXCharPos * l_fontWidth);
        const QBrush oldBrush = painter.brush();

        painter.setBrush(QColor(Qt::black));

        painter.drawRect(xpos, ((m_CarretPosition.m_Row - m_ScrollYLinePos - 1) * l_fontHeight), ceil(m_LineHeight * 0.05), l_fontHeight - 1);
        painter.setBrush(oldBrush);
    }

    if(m_Lines.count() - (viewport()->height() / l_fontHeight) + 1 > 1)
    {
        verticalScrollBar()->setMaximum(m_Lines.count() - (viewport()->height() / l_fontHeight));
    }
    else
    {
        verticalScrollBar()->setMaximum(0);
    }

    int max_LineSize = 0;
    for(const auto &l_Line: std::as_const(m_Lines))
    {
        int l_CurrentLineSize = l_Line.text.length();

        if (l_CurrentLineSize > max_LineSize)
        {
            max_LineSize = l_CurrentLineSize;
        }
    }

    if(max_LineSize - ((viewport()->width() - m_LineNumbersPanelWidth) / l_fontWidth) + 1 > 1)
    {
        horizontalScrollBar()->setMaximum(max_LineSize - ((viewport()->width() - m_LineNumbersPanelWidth) / l_fontWidth));
    }
    else
    {
        horizontalScrollBar()->setMaximum(0);
    }
}

void QCodeEditWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->type() == QEvent::MouseMove)
    {
        if (m_SelectMouseDown)
        {
            m_SelectMouseDown = true;

            killTimer(m_CursorTimerID);
            m_CursorTimerID = startTimer(500);
            m_currentlyBlinkCursorShowen = 1;

            QFontMetrics l_fm(m_TextFont);
            int l_fontHeight = l_fm.height();
            int l_fontWidth = l_fm.horizontalAdvance(' ');

            m_CarretPosition.m_Row = m_ScrollYLinePos + (event->position().y() / l_fontHeight) + 1;

            if (m_CarretPosition.m_Row < 1)
            {
                m_CarretPosition.m_Row = 1;
            }

            if (m_CarretPosition.m_Row > (m_Lines.count()))
            {
                m_CarretPosition.m_Row = m_Lines.count();
            }

            m_CarretPosition.m_Column = (event->position().x() - 30 + (l_fontWidth / 2)) / l_fontWidth;

            if (m_CarretPosition.m_Column < 1)
            {
                m_CarretPosition.m_Column = 1;
            }

            if (m_CarretPosition.m_Column > m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1)
            {
                m_CarretPosition.m_Column = m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1;
            }

            viewport()->update();
        }
    }
}

void QCodeEditWidget::mousePressEvent(QMouseEvent *event)
{
    QFontMetrics l_fm(m_TextFont);
    int l_LineNumbersPanelWidth = 16 + (l_fm.horizontalAdvance(" ") * m_LineNumbersPanelWidth) + 4;

    if (event->position().x() > l_LineNumbersPanelWidth)
    {
        m_SelectMouseDown = true;

        int l_fontHeight = l_fm.height();
        int l_fontWidth = l_fm.horizontalAdvance(' ');

        m_CarretPosition.m_Row = m_ScrollYLinePos + (event->position().y() / l_fontHeight) + 1;

        if (m_CarretPosition.m_Row > (m_Lines.count()))
        {
            m_CarretPosition.m_Row = m_Lines.count();
        }

        m_CarretPosition.m_Column = (event->position().x() - 30 + (l_fontWidth / 2)) / l_fontWidth;

        if (m_CarretPosition.m_Column > m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1)
        {
            m_CarretPosition.m_Column = m_Lines[m_CarretPosition.m_Row - 1].text.length() + 1;
        }

        if (m_CarretPosition.m_Column < 1)
        {
            m_CarretPosition.m_Column = 1;
        }

        m_SelectionPosition.m_Column = m_CarretPosition.m_Column;
        m_SelectionPosition.m_Row = m_CarretPosition.m_Row;

        viewport()->update();
    }
}

void QCodeEditWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_SelectMouseDown = false;

    if ((event->button() == Qt::LeftButton)&&(event->position().x() < 16))
    {
        QFontMetrics l_fm(m_TextFont);
        int l_fontHeight = l_fm.height();

        int l_LineHited = m_ScrollYLinePos + (event->position().y() / l_fontHeight);

        if ((l_LineHited >= 0)&&(l_LineHited < m_Lines.count()))
        {
            emit breakpoint_change(l_LineHited + 1);
        }

        return;
    }
}

void QCodeEditWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier)
    {
        if ((event->angleDelta().ry() > 0)&&(m_FontSize < 48))
        {
            ++m_FontSize;
            m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
            viewport()->update();
        }
        else if ((event->angleDelta().ry() < 0)&&(m_FontSize > 3))
        {
            --m_FontSize;
            m_TextFont = QFont("Source Code Pro", m_FontSize, QFont::Normal, false);
            viewport()->update();
        }
    }
    else
    {
        QAbstractScrollArea::wheelEvent(event);
    }
}

void QCodeEditWidget::focusInEvent(QFocusEvent *event)
{
    m_currentlyBlinkCursorShowen = 1;
    if (m_CursorTimerID == 0)
    {
        m_CursorTimerID = startTimer(500);
    }
    viewport()->update();
}

void QCodeEditWidget::focusOutEvent(QFocusEvent *event)
{
    killTimer(m_CursorTimerID);
    m_CursorTimerID = 0;
    m_currentlyBlinkCursorShowen = 0;
    viewport()->update();
}

QString QCodeEditWidget::text() const
{
    QString ret;

    for(const auto &l_Line: m_Lines)
    {
        ret.append(l_Line.text);

        switch(l_Line.type)
        {
        case QTextParserLine::QTextParserLineTypeCREndLine:
            ret.append('\r');
            break;
        case QTextParserLine::QTextParserLineTypeLFEndLine:
            ret.append('\n');
            break;
        case QTextParserLine::QTextParserLineTypeCRLFEndLine:
            ret.append("\r\n");
            break;
        default:
            break;
        }
    }

    return ret;
}

int QCodeEditWidget::tabSize() const
{
    return m_tabSize;
}

void QCodeEditWidget::setFileExtension(const QString &extension)
{
}

void QCodeEditWidget::setText(QString text)
{
    QCodeEditWidgetLine l_Line;

    m_Lines.clear();
    m_ScrollXCharPos = 0;
    m_ScrollYLinePos = 0;

    int pos = 0;

    l_Line.lineStatus = QCodeEditWidget::QLineStatusTypeLineNotModified;
    l_Line.breakpointType = QCodeEditWidget::QBreakpointTypeNoBreakpoint;

    forever
    {
        int crindex = text.indexOf('\r', pos);
        int lfindex = text.indexOf('\n', pos);

        if ((crindex == -1)&&(lfindex == -1))
        {
            if(pos < text.length() - 1)
            {
                l_Line.text = text.right(text.length() - pos);
                l_Line.type = QTextParserLine::QTextParserLineTypeNoEndLine;

                m_Lines.push_back(l_Line);
            }

            break;
        }

        if ((crindex < lfindex)&&(crindex > -1))
        {
            l_Line.text = text.mid(pos, crindex - pos);
            l_Line.type = QTextParserLine::QTextParserLineTypeCREndLine;
            pos = crindex + 1;

            if((text.length() > pos)&&(text.at(pos) == '\n'))
            {
                l_Line.type = QTextParserLine::QTextParserLineTypeCRLFEndLine;
                pos++;
            }
        }
        else
        {
            l_Line.text = text.mid(pos, lfindex - pos);
            l_Line.type = QTextParserLine::QTextParserLineTypeLFEndLine;
            pos = lfindex + 1;
        }

        m_Lines.push_back(l_Line);
    }

    if ((m_Lines.isEmpty())||(m_Lines.last().type != QTextParserLine::QTextParserLineTypeNoEndLine))
    {
        l_Line.text = "";
        l_Line.type = QTextParserLine::QTextParserLineTypeNoEndLine;
        m_Lines.push_back(l_Line);
    }

    updatePanelWidth();

    emit parserUpdate();

    viewport()->update();
}

void QCodeEditWidget::setTabSize(int size)
{
    if ((size > 1)&&(m_tabSize != size))
    {
        m_tabSize = size;

        viewport()->update();
    }
}

void QCodeEditWidget::clearFormatting()
{
    for(int c = 0; c < m_Lines.count(); c++)
    {
        m_Lines[c].colors.clear();
    }

    viewport()->update();
}

void QCodeEditWidget::addFormat(int line, const QTextParserColorItem &item)
{
    m_Lines[line].colors.append(item);
}

void QCodeEditWidget::setBreakpoint(int line, QBreakpointType type)
{
    auto l_line = m_Lines.at(line - 1);

    l_line.breakpointType = type;

    m_Lines.replace(line - 1, l_line);

    viewport()->update();
}

void QCodeEditWidget::setDebuggerAtLine(int line)
{
    m_debuggerAtLine = line - 1;

    update();
}

QCodeEditWidget::QBreakpointType QCodeEditWidget::breakpoint(int line) const
{
    return m_Lines.at(line - 1).breakpointType;
}

bool QCodeEditWidget::canUndo() const
{
    return false;
}

bool QCodeEditWidget::canRedo() const
{
    return false;
}

bool QCodeEditWidget::hasSelection() const
{
    return false;
}

void QCodeEditWidget::setLines(const QList<QCodeEditWidgetLine> &lines)
{
    m_Lines = lines;

    viewport()->update();
}

QList<QCodeEditWidgetLine> QCodeEditWidget::lines() const
{
    return m_Lines;
}
