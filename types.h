#ifndef TYPES_H
#define TYPES_H

//QT
#include <QDateTime>
#include <QVector>
#include <QString>
#include <QSet>
#include <QHostAddress>

enum class KLineType: qint64 //Тип свечи
{
    MIN1   = 1 * 60 * 1000,     //1мин свеча
    MIN5   = 5 * 60 * 1000,     //5мин свеча
    MIN15  = 15 * 60 * 1000,    //15мин свеча
    MIN30  = 30 * 60 * 1000,    //30мин свеча
    MIN60  = 60 * 60  * 1000,    //60мин свеча
    HOUR4  = 240 * 60 * 1000,
    HOUR8  = 480 * 60 * 1000,
    DAY1   = 1440 * 60 * 1000,
    WEEK1  = 10080 * 60 * 1000,
    UNKNOW = 0      //неизвестный тип свечи
};

using KLineTypes = QSet<KLineType>;

struct StockExchangeID
{
    QString name;
};

size_t qHash(const StockExchangeID& key, size_t seed);
bool operator==(const StockExchangeID& key1, const StockExchangeID& key2);

struct StockExchangeData //глобальна информация про биржу
{
    KLineTypes klineTypes;
    StockExchangeID id;
};

QString KLineTypeToString(KLineType type);
KLineType stringToKLineType(const QString& type);

struct KLineID
{
    QString symbol;        //название монеты
    KLineType type = KLineType::UNKNOW; //интервал свечи
};

size_t qHash(const KLineID& key, size_t seed);
bool operator==(const KLineID& key1, const KLineID& key2);

struct KLine //данные свечи
{
    KLineID id;
    QDateTime openTime;  //время открытия
    double open = 0.0;   //цена в момент открытия
    double high = 0.0;   //наибольшая свеча
    double low = 0.0;    //наименьшая свеча
    double close = 0.0;  //цена в момент закрытия
    double volume = 0.0; //объем
    QDateTime closeTime; //время закрытия
    double quoteAssetVolume = 0.0;
};

struct StockExchangeKLine
{
    StockExchangeID stockExcahnge; //Название биржи
    KLine kline;
};

double deltaKLine(const KLine &kline);
double volumeKLine(const KLine &kline);

using KLines = QVector<KLine>;

#endif // TYPES_H
