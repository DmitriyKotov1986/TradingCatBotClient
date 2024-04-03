#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QChartView>
#include <QCandlestickSeries>
#include <QListWidgetItem>
#include <QSet>
#include <QMap>
#include <QHash>
#include <QComboBox>

#include "httpsquery.h"
#include "localconfig.h"
#include "types.h"
#include "filter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    enum class HTTPRequstType: quint8
    {
        NONE = 0,
        LOGIN = 1,
        KLINES = 2,
        CONFIG = 3,
        NEWUSER = 4,
        DATA = 5
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    void login(const QString& title);

    void getAnswerHttp(const QByteArray& answer, int id);
    void errorOccurredHttp(quint32 code, const QString& msg, int id);

    void eventList_itemClicked(QListWidgetItem *item);
    void detectorSplitter_splitterMoved(int pos, int index);

    void addPushButton_clicked();
    void removePushButton_clicked();

    void stockExchangeComboBox_currentIndexChanged(int index);

    void mainTabWidget_currentChanged(int index);

private:
    struct KLineData
    {
        StockExchangeID stockExchangeID;
        double delta = 0.0;
        double volume = 0.0;
        KLines history;
        KLines reviewHistory;
    };

    using ExistIntervals = QSet<QString>;
    using ExistsKLines = QMap<QString, ExistIntervals>;
    using ExistsStockExchange = QMap<QString, ExistsKLines>;

    struct RequestData
    {
        HTTPRequstType type = HTTPRequstType::NONE;
        Common::HTTPSQuery *HTTPSQuery = nullptr;
    };

    using RequestInfo = QHash<int, RequestData>;

private:
    void makeChart();
    void makeReviewChart();
    void makeFilterTab();
    QComboBox* makeStockExchangeComboBox(const QString& stockExchange) const;
    QComboBox* makeMoneyComboBox(const QString& stockExchange, const QString& money) const;
    QComboBox* makeIntervalComboBox(const QString& stockExchange, const QString& money, const QString& interval) const;

    void addFilterRow(const QString& stockExchange, const QString& money, const QString& interval, double delta, double volume);

    void sendLogin(const QString& user, const QString& password);
    void parseLogin(const QByteArray& data);

    void sendGetKLines();
    void parseKLines(const QByteArray& data);

    void sendGetData();
    void parseData(const QByteArray& data);
    void parseMessage(const QJsonArray& messages);

    void sendConfig();
    void parseConfig(const QByteArray& data);

    void sendNewUser(const QString& user, const QString& password);
    void parseNewUser(const QByteArray& data);

    void addKLines(const QJsonArray& jsonKLineList);
    void addKLine(const QJsonObject& jsonKLine);
    void showChart(const KLineData& klineData);
    void showReviewChart(const KLineData& klineData);

    void sendHTTPRequest(const QUrl& url, HTTPRequstType type, const QByteArray& data);
    void showRemovePushButton();

private:
    Ui::MainWindow *ui;

    LocalConfig _localCnf;

    Common::HTTPSQuery::Headers _headers; //заголовок HTTP запроса к серверу

    RequestInfo _sentHTTPRequest; //информация о текущих запросах

    QHash<quint64, KLineData*> _klines; //список отфильтрованных свечей поступивших от сервера
    Filter _filter; //текущий фильтр

    quint32 _getErrorHTTPCount = 0; //поличество подряд идущих запросов к серверу закончившихся ошибкой

    ExistsStockExchange _existKLines; // список существующих монет

    QCandlestickSeries *_series = nullptr;
    QCandlestickSeries *_seriesVolume = nullptr;
    QChartView *_chartView = nullptr;

    QCandlestickSeries *_reviewSeries = nullptr;
    QCandlestickSeries *_reviewSeriesVolume = nullptr;
    QChartView *_reviewChartView = nullptr;

    int _sessionID = 0;
};
#endif // MAINWINDOW_H
