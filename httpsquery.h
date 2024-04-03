#ifndef HTTPSQUERY_H
#define HTTPSQUERY_H

#include <QObject>
#include <QUrl>
#include <QHash>
#include <QByteArray>
#include <QTimer>

#include <emscripten/fetch.h>

namespace Common
{


class HTTPSQuery : public QObject
{
    Q_OBJECT

public:
    using Headers = QHash<QByteArray, QByteArray>;

public:
    explicit HTTPSQuery(QObject *parent = nullptr);
    ~HTTPSQuery();

    quint64 send(const QUrl& url, const Headers& headers, const QByteArray& data); //запускает отправку запроса

signals:
    void getAnswer(const QByteArray& answer, int id);
    void errorOccurred(quint32 code, const QString& msg, int id);

private slots:
    void checkResult();

private:
    int _id = 0;

    QTimer *_timer = nullptr;

};

} //namespace Common

#endif // HTTPSQUERY_H
