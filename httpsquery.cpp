//STL
#include <string>
#include <optional>
#include <unordered_map>

//Qt
#include <QMutexLocker>
#include <QMutex>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "httpsquery.h"

using namespace Common;
using namespace emscripten;

static const qint64 MAX_SAVE_RESULT = 600000; //ms
static const qint64 CHECK_INTERVAL = 1000; //ms

struct AnswerData
{
    QByteArray answer;
    QDateTime addTime = QDateTime::currentDateTime();
};

static std::unordered_map<int, AnswerData> answers;
static QMutex answerMutex;

void addAnswerResult(int requestID, AnswerData&& answer)
{
    QMutexLocker<QMutex> locker(&answerMutex);

    answers.emplace(std::move(requestID), std::move(answer));
}

void clearOldAnswerResult()
{
    QMutexLocker<QMutex> locker(&answerMutex);

    for (auto answers_it = answers.begin(); answers_it != answers.end(); ++answers_it)
    {
        if (answers_it->second.addTime.msecsTo(QDateTime::currentDateTime()) > MAX_SAVE_RESULT)
        {
            answers_it = answers.erase(answers_it);
        }
    }
}

std::optional<AnswerData> getAnswerResult(int requestID)
{
    QMutexLocker<QMutex> locker(&answerMutex);

    std::optional<AnswerData> result;
    auto answer_it = answers.find(requestID);
    if (answer_it != answers.end())
    {
        result = std::move(answer_it->second);

        answers.erase(answer_it);
    }

    return result;
}

struct ErrorData
{
    quint32 code;
    QString msg;
    QDateTime addTime = QDateTime::currentDateTime();
};

static std::unordered_map<int, ErrorData> errors;
static QMutex errorMutex;

void addErrorResult(int requestID, ErrorData&& answer)
{
    QMutexLocker<QMutex> locker(&errorMutex);

    errors.emplace(std::move(requestID), std::move(answer));
}

std::optional<ErrorData> getErrorResult(int requestID)
{
    QMutexLocker<QMutex> locker(&errorMutex);

    std::optional<ErrorData> result;
    auto error_it = errors.find(requestID);
    if (error_it != errors.end())
    {
        result = std::move(error_it->second);

        errors.erase(error_it);
    }

    return result;
}

void clearOldErrorsResult()
{
    QMutexLocker<QMutex> locker(&errorMutex);

    for (auto errors_it = errors.begin(); errors_it != errors.end(); ++errors_it)
    {
        if (errors_it->second.addTime.secsTo(QDateTime::currentDateTime()) > MAX_SAVE_RESULT)
        {
            errors_it = errors.erase(errors_it);
        }
    }
}

void downloadSucceeded(emscripten_fetch_t *fetch)
{
    QByteArray answer(fetch->data, fetch->numBytes);

    AnswerData tmp;
    tmp.answer = answer;
    tmp.addTime = QDateTime::currentDateTime();
    addAnswerResult(*(int*)(fetch->userData), std::move(tmp));

    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch)
{
    ErrorData tmp;
    tmp.code = fetch->status;
    tmp.msg = QString("Error code: %1 URL: %2").arg(fetch->status).arg(fetch->url);

    addErrorResult(*(int*)(fetch->userData), std::move(tmp));

    emscripten_fetch_close(fetch); // Also free data on failure.
}

HTTPSQuery::HTTPSQuery(QObject *parent)
    : QObject{parent}
    , _id(0)
{
}

HTTPSQuery::~HTTPSQuery()
{
}

int getID()
{
    static int id = 0;
    static QMutex getIDMutex;

    QMutexLocker<QMutex> locker(&getIDMutex);

    return ++id;
}

quint64 HTTPSQuery::send(const QUrl& url, const HTTPSQuery::Headers& headers, const QByteArray& data)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    if (data.isEmpty())
    {
        strcpy(attr.requestMethod, "GET");
    }
    else
    {
        strcpy(attr.requestMethod, "POST");

        auto *buf = (char*)std::malloc(data.size() + 1);
        std::memcpy(buf ,(void *)data.data(), data.size());
        buf[data.size()] = 0;
        attr.requestData = buf;
        attr.requestDataSize = data.size();
    }
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;

    const std::string url_str = url.toString().toStdString();
    auto fetch = emscripten_fetch(&attr, url_str.data());

    fetch->userData = new int{getID()};
    _id = *(int*)fetch->userData;

    //timer
    _timer = new QTimer();

    connect(_timer, SIGNAL(timeout()), SLOT(checkResult()));

    _timer->start(CHECK_INTERVAL);

 //   qDebug() << _id << "SEND TO:" << url << "DATA:" << data;

    return _id;
}

void HTTPSQuery::checkResult()
{
    auto answerResult =  getAnswerResult(_id);
    if (answerResult.has_value())
    {
        emit getAnswer(answerResult.value().answer, _id);

//        qDebug() << _id << "ANSWER:" << answerResult.value().answer;
    }

    auto errorResult =  getErrorResult(_id);
    if (errorResult.has_value())
    {
        emit errorOccurred(errorResult.value().code, errorResult.value().msg, _id);

//        qDebug() << _id << "ERROR:" << errorResult.value().code << errorResult.value().msg;
    }

    clearOldAnswerResult();
    clearOldErrorsResult();
}



