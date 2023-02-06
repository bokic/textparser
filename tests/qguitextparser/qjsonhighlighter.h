#pragma once

#include <textparser.h>

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QObject>
#include <QList>


class QJsonHighlighter final: public QSyntaxHighlighter
{
public:
    QJsonHighlighter(QObject *parent = nullptr);
    QJsonHighlighter(QTextDocument *parent);

protected:
    virtual void highlightBlock(const QString &text) override;

private:
    void init();
    void updateToken(textparse_token_item_t *token);

    QList<QTextCharFormat> m_formats;
};
