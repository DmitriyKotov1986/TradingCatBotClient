#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>

#include "filter.h"


Filter::Filter(const QJsonArray &JSONFilter)
{
    fromJSON(JSONFilter);
}

Filter::~Filter()
{
    clear();
}

void Filter::fromJSON(const QJsonArray &JSONFilter)
{
    clear();

    for (int i = 0; i < JSONFilter.count(); ++i)
    {
        const auto kline = JSONFilter.at(i).toObject();

        FilterData filterData;
        filterData.stockExchangeID.name = kline["StockExchange"].toString();
        filterData.klineID.symbol = kline["Money"].toString();
        filterData.klineID.type = stringToKLineType(kline["Interval"].toString());
        filterData.delta = kline["Delta"].toDouble();
        filterData.volume = kline["Volume"].toDouble();

        addFilter(filterData);
    }
}

QJsonArray Filter::toJSON() const
{
    QJsonArray json;
    for (const auto& filterData: _filterData)
    {
        QJsonObject kline;

        kline.insert("StockExchange", filterData.stockExchangeID.name);
        kline.insert("Money", filterData.klineID.symbol);
        kline.insert("Interval", KLineTypeToString(filterData.klineID.type));
        kline.insert("Delta", filterData.delta);
        kline.insert("Volume", filterData.volume);

        json.push_back(kline);
    }

    return json;
}

void Filter::fromList(const FilterDataList &filterDataList)
{
    _filterData = filterDataList;
}

const Filter::FilterDataList& Filter::toList() const
{
    return _filterData;
}

void Filter::addFilter(const FilterData& filterData)
{
    _filterData.push_back(filterData);
}

void Filter::clear()
{
    _filterData.clear();
}

QString Filter::errorString()
{
    const QString tmp = _errorString;
    _errorString.clear();

    return tmp;
}

bool Filter::isError() const
{
    return !_errorString.isEmpty();
}
