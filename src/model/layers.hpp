#pragma once

#include <QMetaType>

#include "property.hpp"

namespace model {

enum class LayerType
{
    NullLayer = 3,
    ShapeLayer = 4,
};



class Layer : public Object
{
    Q_OBJECT
    Q_ENUM(LayerType);

public:
    explicit Layer(LayerType type)
        : type{this, "type", "ty", type}
    {}

    // ddd
    // hd
    FixedValueProperty<LayerType> type;
    Property<QString> name{this, "name", "nm", ""};
    // parent
    // stretch
    // transform
    // auto_orient
    Property<float> in_point{this, "in_point", "ip", -1};
    Property<float> out_point{this, "out_point", "ip", -1};
    Property<float> start_time{this, "start_time", "st", -1};
    // blend_mode
    // matte_mode
    Property<int> index{this, "index", "ind", -1};
    // css_class
    // layer_html_id
    // has_masks
    // masks
    // effects
};

namespace detail {
    template<LayerType lt>
    class BaseLayerProps : public Layer
    {
    public:
        BaseLayerProps() : Layer(lt) {}
    };
} // namespace detail


class NullLayer : public detail::BaseLayerProps<LayerType::NullLayer>
{};


class ShapeLayer : public detail::BaseLayerProps<LayerType::ShapeLayer>
{
    // shapes
};



} // namespace model

Q_DECLARE_METATYPE(model::LayerType)
