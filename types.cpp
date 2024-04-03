#include "types.h"

QString KLineTypeToString(KLineType type)
{
    switch (type)
    {
    case KLineType::MIN1: return "1m";
    case KLineType::MIN5: return "5m";
    case KLineType::MIN15: return "15m";
    case KLineType::MIN30: return "30m";
    case KLineType::MIN60: return "60m";
    case KLineType::HOUR4: return "4h";
    case KLineType::HOUR8: return "8h";
    case KLineType::DAY1: return "1d";
    case KLineType::WEEK1: return "1w";
    default:
        Q_ASSERT(false);
    }

    return "UNKNOW";
}

KLineType stringToKLineType(const QString& type)
{
    if (type == "1m")
    {
        return KLineType::MIN1;
    }
    else if (type == "5m")
    {
        return KLineType::MIN5;
    }
    else if (type == "15m")
    {
        return KLineType::MIN15;
    }
    else if (type == "30m")
    {
        return KLineType::MIN30;
    }
    else if (type == "60m")
    {
        return KLineType::MIN60;
    }
    else if (type == "4h")
    {
        return KLineType::HOUR4;
    }
    else if (type == "8h")
    {
        return KLineType::HOUR8;
    }
    else if (type == "1d")
    {
        return KLineType::DAY1;
    }
    else if (type == "1w")
    {
        return KLineType::WEEK1;
    }

    return KLineType::UNKNOW;
}

double deltaKLine(const KLine &kline)
{
    return ((kline.high - kline.low) / kline.low) * 100.0;
}

double volumeKLine(const KLine &kline)
{
    return ((kline.open + kline.close) / 2) * kline.volume;
}

size_t qHash(const KLineID &key, size_t seed)
{
    return qHash(key.symbol) + 47 * static_cast<quint64>(key.type);
}

bool operator==(const KLineID &key1, const KLineID &key2)
{
    return (key1.symbol == key2.symbol) && (key1.type == key2.type);
}

size_t qHash(const StockExchangeID &key, size_t seed)
{
    return qHash(key.name);
}

bool operator==(const StockExchangeID& key1, const StockExchangeID& key2)
{
    return key1.name == key2.name;
}

QString TgetKLineTableName(const QString& stockExcangeName, const QString& moneyName, const QString& typeName)
{
    return QString("KLINES_%1_%2_%3").arg(stockExcangeName).arg(moneyName).arg(typeName);
}
