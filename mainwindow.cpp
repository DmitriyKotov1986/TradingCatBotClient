#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCandlestickSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QDateTimeAxis>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QRandomGenerator64>

#include <limits.h>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "logindialog.h"
#include "Common/common.h"
#include "usermessage.h"

using namespace Common;

static const quint64 SEND_INTERVAL = 5000;
#ifdef QT_NO_DEBUG
static const QString SERVER_URL = "https://tradingcat.ru";
#else
static const QString SERVER_URL = "http://localhost:59923";
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //UI
    ui->setupUi(this);

    //http
    _headers.insert(QByteArray{"Content-Type"}, QByteArray{"application/json"});

    //connect signal-slots
    QObject::connect(ui->eventsList, SIGNAL(itemClicked(QListWidgetItem *)),
                     SLOT(eventList_itemClicked(QListWidgetItem *)));

    QObject::connect(ui->detectorSplitter, SIGNAL(splitterMoved(int, int)),
                     SLOT(detectorSplitter_splitterMoved(int, int)));


    QObject::connect(ui->addPushButton, SIGNAL(clicked()), SLOT(addPushButton_clicked()));
    QObject::connect(ui->removePushButton, SIGNAL(clicked()), SLOT(removePushButton_clicked()));

    QObject::connect(ui->mainTabWidget, SIGNAL(currentChanged(int)), SLOT(mainTabWidget_currentChanged(int)));

    makeChart();
    makeReviewChart();

    if (!_localCnf.splitterPos().isEmpty())
    {
        ui->detectorSplitter->restoreState(_localCnf.splitterPos());
    }

    ui->mainTabWidget->setCurrentIndex(0);

    //start
    QTimer::singleShot(100,
                       [this]()
                       {
                            _reviewChartView->show();
                            _chartView->show();

                            resizeEvent(nullptr);
                       });

    QTimer::singleShot(100, [this](){ login("Please enter your username and password"); });
}

MainWindow::~MainWindow()
{
    Q_CHECK_PTR(ui);

    for (const auto& request: _sentHTTPRequest)
    {
        delete request.HTTPSQuery;
    }

    delete ui;
}

void MainWindow::login(const QString& title)
{   
    if (_localCnf.user().isEmpty() || _localCnf.password().isEmpty())
    {
        _localCnf.setUser(QString::number(QRandomGenerator64::global()->generate64()));
        _localCnf.setPassword(QString::number(QRandomGenerator64::global()->generate64()));

        sendNewUser(_localCnf.user(), _localCnf.password());
    }
    else
    {
        sendLogin(_localCnf.user(), _localCnf.password());
    }
}

void MainWindow::
    parseLogin(const QByteArray &data)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "LOGIN: Error parsing json: " << error.errorString();
        Q_ASSERT(false);

        return;
    }

    const auto json = doc.object();
    if (json["Result"] == "OK")
    {
        auto item = new QListWidgetItem(QString("Login successfully as: %1").arg(_localCnf.user()));
        item->setIcon(QIcon(":/image/img/ok.png"));
        ui->eventsList->addItem(item);

        _sessionID = json["SessionID"].toInt(0);

        ui->mainTabWidget->setVisible(true);

        _filter.fromJSON(json["Filter"].toArray());
        if (_filter.isError())
        {
            qDebug() << _filter.errorString();
        }

        QTimer::singleShot(1, [this](){ sendGetKLines(); });
    }
    else
    {
        qDebug() << "LOGIN:" << json["Message"].toString();
        QTimer::singleShot(1, [this, json](){ login(json["Message"].toString()); });
    }
}

void MainWindow::getAnswerHttp(const QByteArray &answer, int id)
{
    const auto sendHTTPRequest_it = _sentHTTPRequest.find(id);
    if (sendHTTPRequest_it == _sentHTTPRequest.end())
    {
        qDebug() << "GET ANSWER UNDEFINE REQUEST WITH ID:" << id;

        return;
    }

    switch (sendHTTPRequest_it.value().type)
    {
    case HTTPRequstType::LOGIN:
        parseLogin(answer);
        break;
    case HTTPRequstType::KLINES:
        parseKLines(answer);
        break;
    case HTTPRequstType::DATA:
        parseData(answer);
        break;
    case HTTPRequstType::CONFIG:
        parseConfig(answer);
        break;
    case HTTPRequstType::NEWUSER:
        parseNewUser(answer);
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    delete sendHTTPRequest_it.value().HTTPSQuery;

    _sentHTTPRequest.erase(sendHTTPRequest_it);

    _getErrorHTTPCount= 0;
}

void MainWindow::errorOccurredHttp(quint32 code, const QString &msg, int id)
{
    const auto sendHTTPRequest_it = _sentHTTPRequest.find(id);
    if (sendHTTPRequest_it == _sentHTTPRequest.end())
    {
        qDebug() << "ERROR UNDEFINE REQUEST WITH ID:" << id;

        return;
    }
    if (_getErrorHTTPCount > 10)
    {
        qDebug() << "SERVER ERROR. Relogin";

        if (sendHTTPRequest_it.value().type == HTTPRequstType::NEWUSER)
        {
            QTimer::singleShot(SEND_INTERVAL, [this](){ sendNewUser(_localCnf.user(), _localCnf.password()); });
        }
        else
        {
            QTimer::singleShot(SEND_INTERVAL, [this](){ sendLogin(_localCnf.user(), _localCnf.password()); });
        }

        _getErrorHTTPCount = 0;

        delete sendHTTPRequest_it.value().HTTPSQuery;

        _sentHTTPRequest.erase(sendHTTPRequest_it);

        auto item = new QListWidgetItem(QString("Connection is lost. Please wait for relogin...").arg(_localCnf.user()));
        item->setIcon(QIcon(":/icon/img/error.ico"));
        ui->eventsList->addItem(item);

        return;
    }

    switch (sendHTTPRequest_it.value().type)
    {
    case HTTPRequstType::LOGIN:
        QTimer::singleShot(SEND_INTERVAL, [this](){ sendLogin(_localCnf.user(), _localCnf.password()); });
        break;
    case HTTPRequstType::KLINES:
        QTimer::singleShot(SEND_INTERVAL, [this](){ sendGetKLines(); });
        break;
    case HTTPRequstType::DATA:
        QTimer::singleShot(SEND_INTERVAL, [this](){ sendGetData(); });
        break;
    case HTTPRequstType::CONFIG:
        QTimer::singleShot(SEND_INTERVAL, [this](){ sendConfig(); });
        break;
    case HTTPRequstType::NEWUSER:
        QTimer::singleShot(SEND_INTERVAL, [this](){ sendNewUser(_localCnf.user(), _localCnf.password()); });
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    delete sendHTTPRequest_it.value().HTTPSQuery;

    _sentHTTPRequest.erase(sendHTTPRequest_it);

    ++_getErrorHTTPCount;

}

void MainWindow::eventList_itemClicked(QListWidgetItem *item)
{
    if (item->data(Qt::UserRole).isNull())
    {
        return;
    }

    showChart(*_klines.value(item->data(Qt::UserRole).toULongLong()));
    showReviewChart(*_klines.value(item->data(Qt::UserRole).toULongLong()));
}

void MainWindow::detectorSplitter_splitterMoved(int pos, int index)
{
    resizeEvent(nullptr);
}

void MainWindow::addPushButton_clicked()
{
    addFilterRow(_existKLines.firstKey(), "ALL", "1m", 5.0, 1000.0);
}

void MainWindow::removePushButton_clicked()
{
    auto& filterTable = ui->filterTableWidget;
    filterTable->removeRow(filterTable->currentRow());

    showRemovePushButton();
}

void MainWindow::stockExchangeComboBox_currentIndexChanged(int index)
{
    const auto currentRow = ui->filterTableWidget->currentRow();
    if (currentRow == 0)
    {
        return;
    }

    auto stockExchangeComboBox = static_cast<QComboBox*>(ui->filterTableWidget->cellWidget(currentRow, 0));

    ui->filterTableWidget->removeCellWidget(currentRow, 1);
    ui->filterTableWidget->setCellWidget(currentRow, 1, makeMoneyComboBox(stockExchangeComboBox->currentText(), "ALL"));
}

void MainWindow::mainTabWidget_currentChanged(int index)
{
    if (index != 0 || _sessionID == 0)
    {
        return;
    }

    _filter.clear();
    for (int row = 0; row < ui->filterTableWidget->rowCount(); ++row)
    {
        Filter::FilterData tmp;

        tmp.stockExchangeID.name = static_cast<QComboBox*>(ui->filterTableWidget->cellWidget(row, 0))->currentText();

        tmp.klineID.symbol = static_cast<QComboBox*>(ui->filterTableWidget->cellWidget(row, 1))->currentText();
        tmp.klineID.type = stringToKLineType(static_cast<QComboBox*>(ui->filterTableWidget->cellWidget(row, 2))->currentText());

        tmp.delta = static_cast<QDoubleSpinBox*>(ui->filterTableWidget->cellWidget(row, 3))->value();
        tmp.volume = static_cast<QDoubleSpinBox*>(ui->filterTableWidget->cellWidget(row, 4))->value();

        _filter.addFilter(tmp);
    }

    sendConfig();
}

void MainWindow::makeChart()
{
    //Chart
    _series = new QCandlestickSeries;
    _series->setIncreasingColor(QColor(Qt::green));
    _series->setDecreasingColor(QColor(Qt::red));
    _series->setBodyOutlineVisible(true);
    _series->setBodyWidth(0.7);
    _series->setCapsVisible(true);
    _series->setCapsWidth(0.5);
    _series->setMinimumColumnWidth(-1.0);
    _series->setMaximumColumnWidth(50.0);
    auto pen = _series->pen();
    pen.setColor(Qt::white);
    _series->setPen(pen);

    _seriesVolume = new QCandlestickSeries;
    _seriesVolume->setIncreasingColor(QColor(61, 56, 70));
    _seriesVolume->setDecreasingColor(QColor(61, 56, 70));
    _seriesVolume->setBodyOutlineVisible(false);
    _seriesVolume->setCapsVisible(true);
    _seriesVolume->setMinimumColumnWidth(-1.0);
    _seriesVolume->setMaximumColumnWidth(50.0);
    _seriesVolume->setCapsWidth(0.5);

    auto chart = new QChart;
    chart->addSeries(_seriesVolume);
    chart->addSeries(_series);
    chart->setTitle("No data");
 //   chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundBrush(QBrush(QColor(36, 31, 49)));
    auto titleFont = chart->titleFont();
    titleFont.setPointSize(20);
    chart->setTitleFont(titleFont);
    chart->legend()->setVisible(false);
    chart->setTitle("No data");
    chart->setMargins({0,0,0,0});

    auto axisX = new QDateTimeAxis;
    axisX->setTickCount(3);
    axisX->setFormat("hh:mm");
    axisX->setGridLineVisible(false);
    axisX->setLabelsColor(Qt::white);

    chart->addAxis(axisX, Qt::AlignBottom);

    _series->attachAxis(axisX);
    _seriesVolume->attachAxis(axisX);

    auto axisY = new QValueAxis();
    axisY->setGridLineVisible(true);
    axisY->setGridLineColor(QColor(94, 92, 100));
    auto axisYGridLinePen = axisY->gridLinePen();
    axisYGridLinePen.setStyle(Qt::DotLine);
    axisY->setGridLinePen(axisYGridLinePen);
    axisY->setLabelsColor(Qt::white);

    chart->addAxis(axisY, Qt::AlignRight);

    _series->attachAxis(axisY);

    auto axisY2 = new QValueAxis();
    axisY2->setGridLineVisible(false);
    axisY2->setLabelsVisible(false);

    chart->addAxis(axisY2, Qt::AlignLeft);

    _seriesVolume->attachAxis(axisY2);

    _chartView = new QChartView(chart, ui->chartFrame);
    _chartView->resize(ui->chartFrame->size());
    _chartView->show();
}

void MainWindow::makeReviewChart()
{
    //ReviewChart
    _reviewSeries = new QCandlestickSeries;
    _reviewSeries->setIncreasingColor(QColor(Qt::green));
    _reviewSeries->setDecreasingColor(QColor(Qt::red));
    _reviewSeries->setBodyOutlineVisible(true);
    _reviewSeries->setBodyWidth(0.5);
    _reviewSeries->setMinimumColumnWidth(-1.0);
    _reviewSeries->setMaximumColumnWidth(50.0);
    _reviewSeries->setCapsVisible(true);
    _reviewSeries->setCapsWidth(0.3);
    auto pen = _series->pen();
    pen.setWidth(1);
    pen.setColor(Qt::white);
    _reviewSeries->setPen(pen);

    _reviewSeriesVolume = new QCandlestickSeries;
    _reviewSeriesVolume->setIncreasingColor(QColor(61, 56, 70));
    _reviewSeriesVolume->setDecreasingColor(QColor(61, 56, 70));
    _reviewSeriesVolume->setBodyOutlineVisible(false);
    _reviewSeriesVolume->setCapsVisible(true);
    _reviewSeriesVolume->setMinimumColumnWidth(-1.0);
    _reviewSeriesVolume->setMaximumColumnWidth(50.0);
    _reviewSeriesVolume->setCapsWidth(0.3);

    auto chart = new QChart;
    chart->addSeries(_reviewSeriesVolume);
    chart->addSeries(_reviewSeries);
 //   chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundBrush(QBrush(QColor(36, 31, 49)));
    auto titleFont = chart->titleFont();
    titleFont.setPointSize(20);
    chart->setTitleFont(titleFont);
    chart->legend()->setVisible(false);
    chart->setTitle("No data");
    chart->setMargins({0,0,0,0});

    auto axisX = new QDateTimeAxis;
    axisX->setTickCount(3);
    axisX->setFormat("hh:mm");
    axisX->setGridLineVisible(false);
    axisX->setLabelsColor(Qt::white);

    chart->addAxis(axisX, Qt::AlignBottom);

    _reviewSeries->attachAxis(axisX);
    _reviewSeriesVolume->attachAxis(axisX);

    auto axisY = new QValueAxis();
    axisY->setGridLineVisible(true);
    axisY->setGridLineColor(QColor(94, 92, 100));
    auto axisYGridLinePen = axisY->gridLinePen();
    axisYGridLinePen.setStyle(Qt::DotLine);
    axisY->setGridLinePen(axisYGridLinePen);
    axisY->setLabelsColor(Qt::white);

    chart->addAxis(axisY, Qt::AlignRight);

    _reviewSeries->attachAxis(axisY);

    auto axisY2 = new QValueAxis();
    axisY2->setGridLineVisible(false);
    axisY2->setLabelsVisible(false);

    chart->addAxis(axisY2, Qt::AlignLeft);

    _reviewSeriesVolume->attachAxis(axisY2);

    _reviewChartView = new QChartView(chart, ui->reviewChartFrame);
    _reviewChartView->resize(ui->reviewChartFrame->size());
    _reviewChartView->show();
}

void MainWindow::makeFilterTab()
{
    ui->filterTableWidget->clear();
    ui->filterTableWidget->setRowCount(0);

    ui->filterTableWidget->setColumnCount(5);
    QStringList headersLabel;
    headersLabel << "Stock exchange" << "Money" << "Interval" << "Delta" << "Volume";
    ui->filterTableWidget->setHorizontalHeaderLabels(headersLabel);
    ui->filterTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->filterTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->filterTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->filterTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->filterTableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    const auto filterList = _filter.toList();
    bool findUnsupportValue = false;
    for (int row = 0; row < filterList.size(); ++row)
    {
        const auto currentStockExcangeName = filterList.at(row).stockExchangeID.name;
        const auto currentMoneyName = filterList.at(row).klineID.symbol;
        const auto currentIntervalName = KLineTypeToString(filterList.at(row).klineID.type);
        const double currentDelta = filterList.at(row).delta;
        const double currentVolume = filterList.at(row).volume;

        addFilterRow(currentStockExcangeName, currentMoneyName, currentIntervalName, currentDelta, currentVolume);
    }

    ui->addPushButton->setEnabled(true);
}

QComboBox *MainWindow::makeStockExchangeComboBox(const QString &stockExchange) const
{
    auto stockExchangeComboBox = new QComboBox();
    stockExchangeComboBox->setEditable(false);

    for (auto existKLines_it = _existKLines.begin(); existKLines_it != _existKLines.end(); ++existKLines_it)
    {
        if (existKLines_it.key() == "MEXC")
        {
            stockExchangeComboBox->addItem(QIcon(":/icon/img/MEXC.ico"), existKLines_it.key());
        }
        else if (existKLines_it.key() == "KUCOIN")
        {
            stockExchangeComboBox->addItem(QIcon(":/icon/img/kucoin.png"), existKLines_it.key());
        }
        else if (existKLines_it.key() == "GATE")
        {
            stockExchangeComboBox->addItem(QIcon(":/icon/img/gate.png"), existKLines_it.key());
        }
        else
        {
            stockExchangeComboBox->addItem(existKLines_it.key());
        }
    }

    auto currentStockExchange_it = _existKLines.find(stockExchange);
    if (currentStockExchange_it == _existKLines.end())
    {
        qDebug() << "Unsupport stock exchange:" << stockExchange << ". Set MEXC";
        stockExchangeComboBox->setCurrentText("MEXC");

        return stockExchangeComboBox;
    }
    else
    {
        stockExchangeComboBox->setCurrentText(stockExchange);
    }

    QObject::connect(stockExchangeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(stockExchangeComboBox_currentIndexChanged(int)));

    return stockExchangeComboBox;
}

QComboBox *MainWindow::makeMoneyComboBox(const QString &stockExchange, const QString& money) const
{
    Q_ASSERT(!stockExchange.isEmpty());
    Q_ASSERT(!money.isEmpty());

    auto moneyComboBox = new QComboBox();
    moneyComboBox->setEditable(false);

    auto currentStockExchange_it = _existKLines.find(stockExchange);
    if (currentStockExchange_it == _existKLines.end())
    {
        return moneyComboBox;
    }

    const auto& moneyList = currentStockExchange_it.value();
    for (auto moneyList_it = moneyList.begin(); moneyList_it != moneyList.end(); ++moneyList_it)
    {
        moneyComboBox->addItem(moneyList_it.key());
    }

    auto currentMoneyList_it = moneyList.find(money);
    if (currentMoneyList_it == moneyList.end())
    {
        qDebug() << "Unsupport money:" << money << ". Set ALL";
        moneyComboBox->setCurrentText("ALL");
    }
    else
    {
        moneyComboBox->setCurrentText(money);
    }

    return moneyComboBox;
}

QComboBox *MainWindow::makeIntervalComboBox(const QString &stockExchange, const QString &money, const QString &interval) const
{
    auto intervalComboBox = new QComboBox();
    intervalComboBox->setEditable(false);

    auto currentStockExchange_it = _existKLines.find(stockExchange);
    if (currentStockExchange_it == _existKLines.end())
    {
        return intervalComboBox;
    }

    const auto& moneyList = currentStockExchange_it.value();
    auto currentMoneyList_it = moneyList.find(money);
    if (currentMoneyList_it == moneyList.end())
    {
        return intervalComboBox;
    }

    const auto& intervalList = currentMoneyList_it.value();
    for (auto intervalList_it = intervalList.begin(); intervalList_it != intervalList.end(); ++intervalList_it)
    {
        intervalComboBox->addItem(*intervalList_it);
    }

    auto currentIntervalList_it = intervalList.find(interval);
    if (currentIntervalList_it == intervalList.end())
    {
        qDebug() << "Unsupport interval:" << interval << ". Set 1min";

        intervalComboBox->setCurrentText(KLineTypeToString(KLineType::MIN1));
        return intervalComboBox;
    }

    intervalComboBox->setCurrentText(interval);

    return intervalComboBox;
}


void MainWindow::sendLogin(const QString &user, const QString &password)
{
    const QString url = QString("%1/login/%2/%3")
                            .arg(SERVER_URL)
                            .arg(user.toUtf8().toBase64(QByteArray::Base64Encoding))
                            .arg(password.toUtf8().toBase64(QByteArray::Base64Encoding));

    sendHTTPRequest(url, HTTPRequstType::LOGIN, QByteArray());
}

void MainWindow::sendGetKLines()
{
    const QString url = QString("%1/klines/%2").arg(SERVER_URL).arg(_sessionID);

    sendHTTPRequest(url, HTTPRequstType::KLINES, QByteArray());
}

void MainWindow::parseKLines(const QByteArray &data)
{  
    _existKLines.clear();

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "KLINE: Error parsing json: " << error.errorString();
        Q_ASSERT(false);

        return;
    }

    const auto json = doc.object();
    if (json["Result"].toString() != "OK")
    {
        qDebug() << "KLINE:" << json["Message"].toString();

        return;
    }

    const auto KLineArrayJson = json["KLines"].toArray();
    for (int i = 0; i < KLineArrayJson.count(); ++i)
    {
        const auto jsonKlines = KLineArrayJson[i].toObject();

        auto& currentMoney = _existKLines[jsonKlines["StockExchange"].toString()][jsonKlines["Money"].toString()];
        currentMoney.insert(jsonKlines["Interval"].toString());
    }

    makeFilterTab();

    QTimer::singleShot(SEND_INTERVAL, [this](){ sendGetData(); });
}

void MainWindow::sendGetData()
{
    const QString url = QString("%1/data/%2").arg(SERVER_URL).arg(_sessionID);

    sendHTTPRequest(url, HTTPRequstType::DATA, QByteArray());
}

void MainWindow::parseData(const QByteArray &data)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "DATA: Error parsing json: " << error.errorString();
        Q_ASSERT(false);

        return;
    }

    const auto json = doc.object();
    if (json["Result"].toString() == "OK")
    {
        addKLines(json["DetectKLines"].toArray());
        if (json.contains("UserMessages"))
        {
            parseMessage(json["UserMessages"].toArray());
        }

        QTimer::singleShot(SEND_INTERVAL, [this, json](){ sendGetData(); });
    }
    else if (json["Result"].toString() == "LOGOUT" )
    {
        qDebug() << "DATA LOGOUT:" << json["Message"].toString();

        QTimer::singleShot(SEND_INTERVAL,
                           [this, json]()
                           {
                               sendLogin(_localCnf.user(), _localCnf.password());
                           });
    }
    else
    {
        qDebug() << "DATA:" << json["Message"].toString();
    }
}

void MainWindow::parseMessage(const QJsonArray &messages)
{
    const auto messageCount = messages.count();
    for (auto i = 0; i < messageCount; ++i)
    {
        const auto message = messages.at(i).toObject();
        if (message["Level"].toString() == "INFO")
        {
            auto item = new QListWidgetItem(message["Message"].toString());
            item->setIcon(QIcon(":/image/img/info.png"));
            ui->eventsList->addItem(item);
        }
    }
}

void MainWindow::sendConfig()
{
    const QString url = QString("%1/config/%2").arg(SERVER_URL).arg(_sessionID);

    QJsonObject json;
    json.insert("Filter", _filter.toJSON());

    sendHTTPRequest(url, HTTPRequstType::CONFIG, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void MainWindow::parseConfig(const QByteArray &data)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "CONFIG: Error parsing json: " << error.errorString();
        Q_ASSERT(false);

        return;
    }

    const auto json = doc.object();
    if (json["Result"].toString() != "OK")
    {
        qDebug() << "CONFIG:" << json["Message"].toString();

        return;
    }
    else
    {
        qDebug() << "CONFIG: configuration applied successfully";
    }
}

void MainWindow::sendNewUser(const QString &user, const QString &password)
{
    const QString url = QString("%1/newuser/%2/%3")
                            .arg(SERVER_URL)
                            .arg(user.toUtf8().toBase64(QByteArray::Base64Encoding))
                            .arg(password.toUtf8().toBase64(QByteArray::Base64Encoding));

    sendHTTPRequest(url, HTTPRequstType::NEWUSER, QByteArray());
}

void MainWindow::parseNewUser(const QByteArray &data)
{
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "NEW USER: Error parsing json: " << error.errorString();
        Q_ASSERT(false);

        return;
    }

    const auto json = doc.object();
    if (json["Result"].toString() != "OK")
    {
        qDebug() << "NEW USER:" << json["Message"].toString();

        _localCnf.setUser(QString::number(QRandomGenerator64::global()->generate64()));
        _localCnf.setPassword(QString::number(QRandomGenerator64::global()->generate64()));

        sendNewUser(_localCnf.user(), _localCnf.password());
    }
    else
    {
        qDebug() << "NEW USER: user added successfully";

        sendLogin(_localCnf.user(), _localCnf.password());
    }
}

void MainWindow::addKLines(const QJsonArray &jsonKLineList)
{
    for (int i = 0; i < jsonKLineList.count(); ++i)
    {
        addKLine(jsonKLineList.at(i).toObject());
    }
}

void MainWindow::addKLine(const QJsonObject &jsonKLine)
{
    auto kline = new KLineData;
    kline->stockExchangeID.name = jsonKLine["StockExchange"].toString();
    kline->delta = jsonKLine["Delta"].toDouble();
    kline->volume = jsonKLine["Volume"].toDouble();

    QJsonArray history = jsonKLine["History"].toArray();
    for (int i = 0; i < history.count(); ++i)
    {
        auto jsonKLine = history.at(i).toObject();
        KLine tmp;
        tmp.id.symbol = jsonKLine["Money"].toString();
        tmp.id.type = stringToKLineType(jsonKLine["Interval"].toString());
        tmp.openTime = QDateTime::fromString(jsonKLine["OpenTime"].toString(), DATETIME_FORMAT);
        tmp.closeTime = QDateTime::fromString(jsonKLine["CloseTime"].toString(), DATETIME_FORMAT);
        tmp.open = jsonKLine["Open"].toDouble();
        tmp.close = jsonKLine["Close"].toDouble();
        tmp.high = jsonKLine["High"].toDouble();
        tmp.low = jsonKLine["Low"].toDouble();
        tmp.volume = jsonKLine["Volume"].toDouble();
        tmp.quoteAssetVolume = jsonKLine["QuoteAssetVolume"].toDouble();

        kline->history.emplaceBack(std::move(tmp));
    }

    if (kline->history.isEmpty())
    {
        qDebug() << jsonKLine;

        Q_ASSERT(false);

        return;
    }

    QJsonArray reviewHistory = jsonKLine["ReviewHistory"].toArray();
    for (int i = 0; i < reviewHistory.count(); ++i)
    {
        auto jsonKLine = reviewHistory.at(i).toObject();
        KLine tmp;
        tmp.id.symbol = jsonKLine["Money"].toString();
        tmp.id.type = stringToKLineType(jsonKLine["Interval"].toString());
        tmp.openTime = QDateTime::fromString(jsonKLine["OpenTime"].toString(), DATETIME_FORMAT);
        tmp.closeTime = QDateTime::fromString(jsonKLine["CloseTime"].toString(), DATETIME_FORMAT);
        tmp.open = jsonKLine["Open"].toDouble();
        tmp.close = jsonKLine["Close"].toDouble();
        tmp.high = jsonKLine["High"].toDouble();
        tmp.low = jsonKLine["Low"].toDouble();
        tmp.volume = jsonKLine["Volume"].toDouble();
        tmp.quoteAssetVolume = jsonKLine["QuoteAssetVolume"].toDouble();

        kline->reviewHistory.emplaceBack(std::move(tmp));
    }

    const  quint64 id = _klines.size();
    _klines.insert(id, kline);

    const QString text = QString("%1->%2 Interval: %3 Delta=%4 Volume=%5")
                             .arg(kline->stockExchangeID.name)
                             .arg(kline->history.first().id.symbol)
                             .arg(KLineTypeToString(kline->history.first().id.type))
                             .arg(kline->delta)
                             .arg(kline->volume);

    auto item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, id);
    if (kline->stockExchangeID.name == "MEXC")
    {
        item->setForeground(Qt::yellow);
    }
    else if (kline->stockExchangeID.name == "KUCOIN")
    {
        item->setForeground(QColor(246, 97, 81));
    }
    else if (kline->stockExchangeID.name == "GATE")
    {
        item->setForeground(QColor(153, 193, 241));
    }

    if (kline->history.first().open <= kline->history.first().close)
    {
        item->setIcon(QIcon(":/image/img/increase.png"));
    }
    else
    {
        item->setIcon(QIcon(":/image/img/decrease.png"));
    }

    ui->eventsList->addItem(item);
    ui->eventsList->setCurrentItem(item);

    showChart(*kline);
    showReviewChart(*kline);
}

void MainWindow::showChart(const KLineData &klineData)
{
    if (_chartView == nullptr)
    {
        makeChart();
    }

    Q_CHECK_PTR(_series);
    Q_CHECK_PTR(_chartView);

    _series->clear();
    _seriesVolume->clear();

    _chartView->chart()->setTitle(QString("%1: %2 %3")
                                            .arg(klineData.stockExchangeID.name)
                                            .arg(klineData.history.first().id.symbol)
                                            .arg(KLineTypeToString(klineData.history.first().id.type)));

    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();
    double maxVolume = std::numeric_limits<double>::min();

    for (auto kline_it = klineData.history.begin(); kline_it != klineData.history.end(); ++kline_it)
    {
        auto candlestick = new QCandlestickSet(kline_it->closeTime.toMSecsSinceEpoch());
        candlestick->setHigh(kline_it->high);
        candlestick->setLow(kline_it->low);
        candlestick->setOpen(kline_it->open);
        candlestick->setClose(kline_it->open != kline_it->close ? kline_it->close : kline_it->close + 0.001 * kline_it->close);
        auto pen = _series->pen();
        pen.setColor(kline_it->open <= kline_it->close ? QColor(Qt::green) : QColor(Qt::red));
        candlestick->setPen(pen);

        _series->append(candlestick);

        auto candlestickVolume = new QCandlestickSet(kline_it->closeTime.toMSecsSinceEpoch());
        candlestickVolume->setOpen(kline_it->volume);
        candlestickVolume->setHigh(kline_it->volume);
        candlestickVolume->setLow(0);
        candlestickVolume->setClose(0);
        auto penVolume = _seriesVolume->pen();
        penVolume.setColor(QColor(61, 56, 70));
        candlestickVolume->setPen(penVolume);

        _seriesVolume->append(candlestickVolume);

        max = std::max(max, kline_it->high);
        min = std::min(min, kline_it->low);
        maxVolume = std::max(maxVolume, kline_it->volume);
    }

    auto axisX = qobject_cast<QDateTimeAxis*>(_chartView->chart()->axes(Qt::Horizontal).at(0));
    axisX->setMax(klineData.history.first().closeTime.addMSecs(static_cast<qint64>(klineData.history.first().id.type) * 5));
    axisX->setMin(klineData.history.last().closeTime.addMSecs(-static_cast<qint64>(klineData.history.first().id.type)));
    axisX->setTickCount(5);

    auto axisY = qobject_cast<QValueAxis*>(_chartView->chart()->axes(Qt::Vertical).at(0));
    axisY->setMax(max * 1.05);
    axisY->setMin(min * 0.95);

    auto axisY2 = qobject_cast<QValueAxis*>(_chartView->chart()->axes(Qt::Vertical).at(1));
    axisY2->setMax(maxVolume * 2);
    axisY2->setMin(0);

    _chartView->show();
}

void MainWindow::showReviewChart(const KLineData &klineData)
{
    if (_reviewChartView == nullptr)
    {
        makeReviewChart();
    }

    Q_CHECK_PTR(_reviewSeries);
    Q_CHECK_PTR(_reviewChartView);

    _reviewSeries->clear();
    _reviewSeriesVolume->clear();

    if (klineData.reviewHistory.isEmpty())
    {
        _reviewChartView->chart()->setTitle("No data");
        _reviewChartView->show();

        return;
    }

    _reviewChartView->chart()->setTitle(QString("%2 %3")
                                            .arg(klineData.reviewHistory.first().id.symbol)
                                            .arg(KLineTypeToString(klineData.reviewHistory.first().id.type)));

    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();
    double maxVolume = std::numeric_limits<double>::min();

    for (auto kline_it = klineData.reviewHistory.begin(); kline_it != klineData.reviewHistory.end(); ++kline_it)
    {
        auto candlestick = new QCandlestickSet(kline_it->closeTime.toMSecsSinceEpoch());

        candlestick->setHigh(kline_it->high);
        candlestick->setLow(kline_it->low);
        candlestick->setOpen(kline_it->open);
        candlestick->setClose(kline_it->close);
        auto pen = _series->pen();
        pen.setColor(kline_it->open <= kline_it->close ? QColor(Qt::green) : QColor(Qt::red));
        candlestick->setPen(pen);

        _reviewSeries->append(candlestick);

        auto candlestickVolume = new QCandlestickSet(kline_it->closeTime.toMSecsSinceEpoch());
        candlestickVolume->setOpen(kline_it->volume);
        candlestickVolume->setHigh(kline_it->volume);
        candlestickVolume->setLow(0);
        candlestickVolume->setClose(0);
        auto penVolume = _seriesVolume->pen();
        penVolume.setColor(QColor(61, 56, 70));
        candlestickVolume->setPen(penVolume);

        _reviewSeriesVolume->append(candlestickVolume);


        max = std::max(max, kline_it->high);
        min = std::min(min, kline_it->low);
        maxVolume = std::max(maxVolume, kline_it->volume);
    }

    auto axisX = qobject_cast<QDateTimeAxis*>(_reviewChartView->chart()->axes(Qt::Horizontal).at(0));
    axisX->setMax(klineData.reviewHistory.first().closeTime.addMSecs(static_cast<qint64>(klineData.reviewHistory.first().id.type) * 5));
    axisX->setMin(klineData.reviewHistory.last().closeTime.addMSecs(-static_cast<qint64>(klineData.reviewHistory.first().id.type)));
    axisX->setTickCount(5);

    auto axisY = qobject_cast<QValueAxis*>(_reviewChartView->chart()->axes(Qt::Vertical).at(0));
    axisY->setMax(max * 1.05);
    axisY->setMin(min * 0.95);

    auto axisY2 = qobject_cast<QValueAxis*>(_reviewChartView->chart()->axes(Qt::Vertical).at(1));
    axisY2->setMax(maxVolume * 2);
    axisY2->setMin(0);

    _reviewChartView->show();
}

void MainWindow::sendHTTPRequest(const QUrl &url, HTTPRequstType type, const QByteArray& data)
{
    RequestData request;
    request.HTTPSQuery = new HTTPSQuery();
    request.type = type;

    QObject::connect(request.HTTPSQuery, SIGNAL(getAnswer(const QByteArray&, int)),
                     SLOT(getAnswerHttp(const QByteArray&, int)));
    QObject::connect(request.HTTPSQuery, SIGNAL(errorOccurred(quint32, const QString&, int)),
                     SLOT(errorOccurredHttp(quint32, const QString&, int)));

    _sentHTTPRequest.insert(request.HTTPSQuery->send(url, _headers, data), request);
}

void MainWindow::addFilterRow(const QString &stockExchange, const QString &money, const QString &interval, double delta, double volume)
{
    auto deltaSpinBox = new QDoubleSpinBox();
    deltaSpinBox->setMinimum(2);
    deltaSpinBox->setMaximum(1000000);
    deltaSpinBox->setValue(delta);
    deltaSpinBox->setSpecialValueText("MINIMUM");

    auto volumeSpinBox = new QDoubleSpinBox();
    volumeSpinBox->setMinimum(500);
    volumeSpinBox->setMaximum(1000000);
    volumeSpinBox->setSingleStep(100.0);
    volumeSpinBox->setValue(volume);
    volumeSpinBox->setSpecialValueText("MINIMUM");

    ui->filterTableWidget->insertRow(ui->filterTableWidget->rowCount());
    const auto currentRow = ui->filterTableWidget->rowCount() - 1;
    ui->filterTableWidget->setCellWidget(currentRow, 0, makeStockExchangeComboBox(stockExchange));
    ui->filterTableWidget->setCellWidget(currentRow, 1, makeMoneyComboBox(stockExchange, money));
    ui->filterTableWidget->setCellWidget(currentRow, 2, makeIntervalComboBox(stockExchange, money, interval));
    ui->filterTableWidget->setCellWidget(currentRow, 3, deltaSpinBox);
    ui->filterTableWidget->setCellWidget(currentRow, 4, volumeSpinBox);

    showRemovePushButton();
}

void MainWindow::showRemovePushButton()
{
    ui->removePushButton->setEnabled(ui->filterTableWidget->rowCount() != 0);
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (_chartView != nullptr)
    {
        _chartView->resize(ui->chartFrame->size());
        _chartView->update();
    }

    if (_reviewChartView != nullptr)
    {
        _reviewChartView->resize(ui->reviewChartFrame->size());
        _reviewChartView->update();
    }

    _localCnf.setSplitterPos(ui->detectorSplitter->saveState());
}
