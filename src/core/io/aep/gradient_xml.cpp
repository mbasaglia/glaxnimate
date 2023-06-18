/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "io/aep/gradient_xml.hpp"

#include "io/svg/detail.hpp"

using namespace glaxnimate::io;
using namespace glaxnimate::io::aep;

CosValue aep::xml_value(const QDomElement& element)
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

CosArray aep::xml_array(const QDomElement& element)
{
    auto data = std::make_unique<CosArray::element_type>();

    for ( const auto& child : svg::detail::ElementRange(element) )
    {
        if ( child.tagName() != "array.type" )
            data->push_back(xml_value(child));
    }
    return data;
}

CosObject aep::xml_list(const QDomElement& element)
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

Gradient aep::parse_gradient_xml(const CosValue& value)
{
    Gradient gradient;
    auto& data = get(value, "Gradient Color Data");
    gradient.color_stops = get_gradient_stops<GradientStopColor>(data);
    gradient.alpha_stops = get_gradient_stops<GradientStopAlpha>(data);
    return gradient;
}

Gradient aep::parse_gradient_xml(const QString& xml)
{
    QDomDocument dom;
    dom.setContent(xml.trimmed());
    return parse_gradient_xml(xml_value(dom.documentElement()));
}
