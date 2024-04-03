#ifndef FILTER_H
#define FILTER_H

#include <QString>

#include "types.h"

class Filter
{
public:
    struct FilterData
    {
        StockExchangeID stockExchangeID;
        KLineID klineID;
        double delta = 0;
        double volume = 0;
    };

    using FilterDataList = QList<FilterData>;

public:
    Filter() = default;
    explicit Filter(const QJsonArray& JSONFilter);

    ~Filter();

    void fromJSON(const QJsonArray& JSONFilter);
    QJsonArray toJSON() const;
    void fromList(const FilterDataList& filterDataList);
    const FilterDataList& toList() const;

    void addFilter(const FilterData& filterData);
    void clear();

    QString errorString();
    bool isError() const;

private:
    FilterDataList _filterData;

    QString _errorString;
};

#endif // FILTER_H
