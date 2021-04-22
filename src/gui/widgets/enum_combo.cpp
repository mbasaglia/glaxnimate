#include "enum_combo.hpp"

#include <cstring>

#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/polystar.hpp"
#include "model/shapes/text.hpp"

EnumCombo::EnumCombo(const QMetaEnum& meta_enum, int current_value, QWidget* parent)
    : QComboBox(parent), meta_enum(meta_enum)
{
    populate(current_value);
}

EnumCombo::EnumCombo(QWidget* parent)
    : QComboBox(parent)
{
}

void EnumCombo::set_data(const QMetaEnum& meta_enum, int current_value)
{
    clear();
    this->meta_enum = meta_enum;
    populate(current_value);
}

void EnumCombo::populate(int current_value)
{
    for ( int i = 0; i < meta_enum.keyCount(); i++ )
    {
        auto data = data_for(meta_enum, meta_enum.value(i));
        addItem(QIcon::fromTheme(data.second), data.first, meta_enum.value(i));
        if ( meta_enum.value(i) == current_value )
            setCurrentIndex(count() - 1);
    }
}

std::pair<QString, const char*> EnumCombo::data_for(const QMetaEnum& meta_enum, int value)
{
    if ( std::strcmp(meta_enum.name(), "Rule") == 0 )
    {
        switch ( model::Fill::Rule(value) )
        {
            case model::Fill::NonZero:
                return {tr("NonZero"), "fill-rule-nonzero"};
            case model::Fill::EvenOdd:
                return {tr("Even Odd"), "fill-rule-even-odd"};
        }
    }
    else if ( std::strcmp(meta_enum.name(), "Cap") == 0 )
    {
        switch ( model::Stroke::Cap(value) )
        {
            case model::Stroke::ButtCap:
                return {tr("Butt"), "stroke-cap-butt"};
            case model::Stroke::RoundCap:
                return {tr("Round"), "stroke-cap-round"};
            case model::Stroke::SquareCap:
                return {tr("Square"), "stroke-cap-square"};
        }
    }
    else if ( std::strcmp(meta_enum.name(), "Join") == 0 )
    {
        switch ( model::Stroke::Join(value) )
        {
            case model::Stroke::MiterJoin:
                return {tr("Miter"), "stroke-cap-miter"};
            case model::Stroke::RoundJoin:
                return {tr("Round"), "stroke-join-round"};
            case model::Stroke::BevelJoin:
                return {tr("Bevel"), "stroke-cap-bevel"};
        }
    }
    else if ( std::strcmp(meta_enum.name(), "StarType") == 0 )
    {
        switch ( model::PolyStar::StarType(value) )
        {
            case model::PolyStar::Star:
                return {tr("Star"), "draw-star"};
            case model::PolyStar::Polygon:
                return {tr("Polygon"), "draw-polygon"};
        }
    }
    else if ( std::strcmp(meta_enum.name(), "GradientType") == 0 )
    {
        switch ( model::Gradient::GradientType(value) )
        {
            case model::Gradient::Linear:
                return {tr("Linear"), "paint-gradient-linear"};
            case model::Gradient::Radial:
                return {tr("Radial"), "paint-gradient-radial"};
        }
    }
    else if ( std::strcmp(meta_enum.name(), "Alignment") == 0 )
    {
        switch ( model::Font::Alignment(value) )
        {
            case model::Font::Left:
                return {tr("Left"), "format-justify-left"};
            case model::Font::Right:
                return {tr("Right"), "format-justify-right"};
            case model::Font::Center:
                return {tr("Center"), "format-justify-center"};
        }
    }

    return {meta_enum.valueToKey(value), "paint-unknown"};
}

void EnumCombo::retranslate()
{
    for ( int i = 0; i < count(); i++ )
    {
        setItemText(i, data_for(meta_enum, meta_enum.value(i)).first);
    }
}

int EnumCombo::current_value() const
{
    return itemData(currentIndex()).toInt();
}

void EnumCombo::set_current_value(int value)
{
    for ( int i = 0; i < count(); i++ )
        if ( meta_enum.value(i) == value )
        {
            setCurrentIndex(i);
            break;
        }
}

bool EnumCombo::set_data_from_qvariant(const QVariant& data)
{
    clear();
    int value = 0;
    if ( inspect_qvariant(data, meta_enum, value) )
    {
        set_data(meta_enum,  data.toInt());
        return true;
    }
    return false;
}

bool EnumCombo::inspect_qvariant(const QVariant& data, QMetaEnum& meta_enum, int& value)
{
    const QMetaObject* mo = QMetaType::metaObjectForType(data.userType());
    if ( !mo )
        return false;

    int index = mo->indexOfEnumerator(
        model::detail::naked_type_name(data.typeName()).toStdString().c_str()
    );
    if ( index == -1 )
        return false;

    meta_enum = mo->enumerator(index);
    value = data.toInt();
    return true;
}


std::pair<QString, const char *> EnumCombo::data_for(const QVariant& data)
{
    QMetaEnum meta_enum;
    int value = 0;
    if ( inspect_qvariant(data, meta_enum, value) )
        return data_for(meta_enum, value);

    return {"", "paint-unknown"};
}
