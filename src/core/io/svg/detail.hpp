#pragma once

#include <map>
#include <unordered_set>

#include <QColor>

#include "utils/qstring_hash.hpp"

namespace io::svg::detail {

extern const std::map<QString, QString> xmlns;

struct Style
{
public:
    using Map = std::map<QString, QString>;

    Style(Map&& map) : map(std::move(map)) {}
    Style() = default;

    QString& operator[](const QString& s) { return map[s]; }
    const QString& operator[](const QString& s) const { return map.at(s); }
    const QString& get(const QString& k, const QString& def = {}) const
    {
        auto it = map.find(k);
        if ( it == map.end() )
            return def;
        return it->second;
    }

    bool contains(const QString& k) const { return map.count(k); }

    Map map;
    QColor color = Qt::black;
};

extern const std::unordered_set<QString> css_atrrs;

} // io::svg::detail
