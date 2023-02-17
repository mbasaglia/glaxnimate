#pragma once

#include <map>
#include <unordered_set>

#include <QColor>
#include <QDomNodeList>

#include "app/utils/qstring_hash.hpp"

namespace glaxnimate::io::svg::detail {

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
    void set(const QString& k, const QString& v)
    {
        map[k] = v;
    }

    bool contains(const QString& k) const { return map.count(k); }

    Map map;
    QColor color = Qt::black;
};

extern const std::unordered_set<QString> css_atrrs;


template<class T>
struct ItemCountRange
{
//         using value_type = decltype(std::declval<T>().item(0));
    struct iterator
    {
        auto operator*() const { return range->dom_list.item(index); }
        iterator& operator++() { index++; return *this; }
        bool operator != (const iterator& it) const
        {
            return range != it.range || index != it.index;
        }

        const ItemCountRange* range;
        int index;
    };

    ItemCountRange(const T& dom_list) : dom_list(dom_list) {}
    iterator begin() const { return {this, 0}; }
    iterator end() const { return {this, dom_list.count()}; }
    int size() const { return dom_list.count(); }

    T dom_list;
};

struct ElementRange
{
    struct iterator
    {
        auto operator*() const { return range->dom_list.item(index).toElement(); }
        iterator& operator++()
        {
            index++;
            while ( index < range->dom_list.count() && !range->dom_list.item(index).isElement() )
                index++;
            return *this;

        }
        bool operator != (const iterator& it) const
        {
            return range != it.range || index != it.index;
        }

        const ElementRange* range;
        int index;
    };

    ElementRange(const QDomNodeList& dom_list) : dom_list(dom_list) {}
    iterator begin() const { return {this, 0}; }
    iterator end() const { return {this, dom_list.count()}; }
    int size() const { return dom_list.count(); }

    QDomNodeList dom_list;
};

} // io::svg::detail
