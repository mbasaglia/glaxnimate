/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QDomDocument>
#include <QGradientStops>

#include "cos.hpp"
#include "io/svg/detail.hpp"
#include "math/vector.hpp"

namespace glaxnimate::io::aep {

CosArray xml_array(const QDomElement& element);
CosObject xml_map(const QDomElement& element);
CosObject xml_list(const QDomElement& element);

CosValue xml_value(const QDomElement& element)
{
    if ( element.tagName() == "prop.map" )
        return xml_value(element.firstChildElement());
    else if ( element.tagName() == "prop.list" )
        return xml_list(element);
    else if ( element.tagName() == "array" )
        return xml_array(element);
    else if ( element.tagName() == "int" )
        return element.text().toDouble();
    else if ( element.tagName() == "float" )
        return element.text().toDouble();
    else if ( element.tagName() == "string" )
        return element.text();
    else
        return {};
}


CosArray xml_array(const QDomElement& element)
{
    auto data = std::make_unique<CosArray::element_type>();

    for ( const auto& child : svg::detail::ElementRange(element) )
    {
        if ( child.tagName() != "array.type" )
            data->push_back(xml_value(child));
    }
    return data;
}


CosObject xml_list(const QDomElement& element)
{
    auto data = std::make_unique<CosObject::element_type>();
    for ( const auto& pair : svg::detail::ElementRange(element, "prop.pair") )
    {
        QString key;
        CosValue value;
        for ( const auto& ch : svg::detail::ElementRange(pair) )
        {
            if ( ch.tagName() == "key" )
                key = ch.text();
            else
                value = xml_value(ch);
        }
        data->emplace(key, std::move(value));
    }

    return data;
}

const CosValue& get_value(const CosObject& v, const QString& key)
{
    return v->at(key);
}

const CosValue& get_value(const CosArray& v, int key)
{
    return v->at(key);
}

const CosValue& get_value(const CosValue& v, const QString& key)
{
    return v.get<CosValue::Index::Object>()->at(key);
}

const CosValue& get_value(const CosValue& v, int key)
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


QGradientStops get_gradient_stops(const CosValue& data, bool alpha)
{
    QGradientStops colors;

    QGradientStop previous;
    qreal midpoint = 0;
    bool first = true;
    const char* name = alpha ? "Stops Alpha" : "Stops Color";

    using Stop = std::pair<qreal, CosArray::element_type*>;
    std::vector<Stop> stops;
    for ( auto& stop : *get_as<CosValue::Index::Object>(data, alpha ? "Alpha Stops" : "Color Stops", "Stops List") )
    {
        auto& stop_arr = get(stop.second, name);
        auto ptr = stop_arr.get<CosValue::Index::Array>().get();
        qreal offset = get_as<CosValue::Index::Number>(stop_arr, 0);
        stops.push_back({offset, ptr});
    }

    std::sort(stops.begin(), stops.end(), [](const Stop& a, const Stop& b) {
        return a.first <= b.first;
    });

    for ( auto stop : stops )
    {
        const auto& color_arr = *stop.second;
        qreal pos = stop.first;

        QColor color;
        if ( alpha )
        {
            color = QColor::fromRgbF(1, 1, 1, color_arr.at(2).get<CosValue::Index::Number>());
        }
        else
        {
            color = QColor::fromRgbF(
                color_arr.at(2).get<CosValue::Index::Number>(),
                color_arr.at(3).get<CosValue::Index::Number>(),
                color_arr.at(4).get<CosValue::Index::Number>()
            );
        }

        if ( !first && !qFuzzyCompare(midpoint, 0.5) )
        {
            qreal midoffset = math::lerp(previous.first, pos, midpoint);
            auto midcolor = math::lerp(previous.second, color, midpoint);
            colors.push_back({midoffset, midcolor});
        }

        midpoint = color_arr.at(1).get<CosValue::Index::Number>();
        first = false;
        colors.push_back({pos, color});
    }

    return colors;
}

qreal get_alpha_at(const QGradientStops& alpha_stops, qreal t, int& index)
{
    if ( alpha_stops.size() == 0 )
        return 1;

    if ( alpha_stops.size() == 1 )
        return alpha_stops[0].second.alphaF();

    if ( t >= alpha_stops.back().first || index + 1 >= alpha_stops.size() )
    {
        index = alpha_stops.size();
        return alpha_stops.back().second.alphaF();
    }

    while ( t >= alpha_stops[index+1].first )
        index++;

    if ( index + 1 >= alpha_stops.size() )
        return alpha_stops.back().second.alphaF();

    qreal delta = alpha_stops[index+1].first - alpha_stops[index].first;
    qreal factor = (t - alpha_stops[index].first) / delta;
    return math::lerp(
        alpha_stops[index].second.alphaF(),
        alpha_stops[index+1].second.alphaF(),
        factor
    );
}

QGradientStops parse_gradient_xml(const CosValue& gradient)
{
    QVector<QPair<qreal, qreal>> alpha;

    auto& data = get(gradient, "Gradient Color Data");
    QGradientStops color_stops = get_gradient_stops(data, false);
    QGradientStops alpha_stops = get_gradient_stops(data, true);
    int index = 0;
    for ( auto& stop : color_stops )
    {
        qreal alpha = get_alpha_at(alpha_stops, stop.first, index);
        stop.second.setAlphaF(alpha);
    }
    return color_stops;
}

QGradientStops parse_gradient_xml(const QString& xml)
{
    QDomDocument dom;
    dom.setContent(xml.trimmed());
    return parse_gradient_xml(xml_value(dom.documentElement()));
}


} // namespace glaxnimate::io::aep

