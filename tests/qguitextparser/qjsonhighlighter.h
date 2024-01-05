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
    explicit QJsonHighlighter(QObject *parent = nullptr);

protected:
    virtual void highlightBlock(const QString &text) override;

private:
    void init();
    void updateToken(textparse_token_item *token);

    QList<QTextCharFormat> m_formats;
};
