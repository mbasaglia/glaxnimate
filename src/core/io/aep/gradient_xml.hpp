/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QDomDocument>
#include <QGradientStops>

#include "cos.hpp"
#include "ae_project.hpp"

namespace glaxnimate::io::aep {

CosArray xml_array(const QDomElement& element);
CosObject xml_map(const QDomElement& element);
CosObject xml_list(const QDomElement& element);
CosValue xml_value(const QDomElement& element);

inline const CosValue& get_value(const CosObject& v, const QString& key)
{
    return v->at(key);
}

inline const CosValue& get_value(const CosArray& v, int key)
{
    return v->at(key);
}

inline const CosValue& get_value(const CosValue& v, const QString& key)
{
    return v.get<CosValue::Index::Object>()->at(key);
}

inline const CosValue& get_value(const CosValue& v, int key)
{
    return v.get<CosValue::Index::Array>()->at(key);
}

template<class V>
const CosValue& get(const V& v)
{
    return v;
}

template<class V, class H, class... T>
const CosValue& get(const V& v, const H& key, const T&... keys)
{
    return get(get_value(v, key), keys...);
}

template<CosValue::Index Ind, class V, class... T>
const auto& get_as(const V& v, const T&... keys)
{
    return get(v, keys...).template get<Ind>();
}

struct GradientStopAlpha
{
    static constexpr const char* const name1 = "Alpha Stops";
    static constexpr const char* const name2 = "Stops Alpha";
    using Value = double;

    static Value get(const CosArray::element_type* arr)
    {
        return arr->at(2).get<CosValue::Index::Number>();
    }
};

struct GradientStopColor
{
    static constexpr const char* const name1 = "Color Stops";
    static constexpr const char* const name2 = "Stops Color";
    using Value = QColor;

    static Value get(const CosArray::element_type* arr)
    {
        return QColor::fromRgbF(
            arr->at(2).get<CosValue::Index::Number>(),
            arr->at(3).get<CosValue::Index::Number>(),
            arr->at(4).get<CosValue::Index::Number>()
        );
    }
};

template<class Policy>
GradientStops<typename Policy::Value> get_gradient_stops(const CosValue& data)
{
    using Stop = GradientStop<typename Policy::Value>;
    GradientStops<typename Policy::Value> stops;

    for ( auto& stop : *get_as<CosValue::Index::Object>(data, Policy::name1, "Stops List") )
    {
        auto& stop_arr = get(stop.second, Policy::name2);
        auto ptr = stop_arr.template get<CosValue::Index::Array>().get();
        qreal offset = get_as<CosValue::Index::Number>(stop_arr, 0);
        qreal midpoint = get_as<CosValue::Index::Number>(stop_arr, 1);
        auto value = Policy::get(ptr);
        stops.push_back(Stop{offset, midpoint, value});
    }

    std::sort(stops.begin(), stops.end(), [](const Stop& a, const Stop& b) {
        return a.offset <= b.offset;
    });

    return stops;
}

Gradient parse_gradient_xml(const CosValue& value);

Gradient parse_gradient_xml(const QString& xml);

} // namespace glaxnimate::io::aep
