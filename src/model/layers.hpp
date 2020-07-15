#pragma once

#include <QMetaType>
#include <QColor>

#include "property.hpp"

namespace model {

enum class LayerType
{
    NullLayer = 3,
    ShapeLayer = 4,
};

class Composition;
class Layer;

class ChildLayerView
{
public:
    class iterator
    {
    public:
        iterator& operator++()
        {
            ++index;
            find_first();
            return *this;
        }

        Layer& operator*() const;
        Layer* operator->() const;

        bool operator==(const iterator& other) const
        {
            return comp == other.comp && parent == other.parent && index == other.index;
        }

        bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

    private:
        iterator(const Composition* comp, const Layer* parent, int index)
        : comp(comp),
          parent(parent),
          index(index)
        {
            find_first();
        }

        void find_first();
        friend ChildLayerView;
        const Composition* comp;
        const Layer* parent;
        int index;
    };

    ChildLayerView(const Composition* comp, const Layer* parent);

    iterator begin() const;
    iterator end() const;

private:
    const Composition* comp;
    const Layer* parent;
};



class Layer : public Object
{
    Q_OBJECT
    Q_ENUM(LayerType);

public:
    explicit Layer(Composition* composition, LayerType type)
        : composition(composition), type{this, "type", "ty", type}
    {}

    Composition* composition;

    // ddd
    // hd
    FixedValueProperty<LayerType> type;
    Property<QString> name{this, "name", "nm", ""};
    Property<Layer*> parent{this, "parent", "parent", nullptr};
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
    Property<QColor> group_color{this, "color", "", QColor(1, 1, 1)};

    ChildLayerView children() const
    {
        return ChildLayerView(composition, this);
    }


    std::unique_ptr<Layer> clone_covariant() const
    {
        auto object = std::make_unique<Layer>(composition, type.get());
        clone_into(object.get());
        return object;
    }

private:
    std::unique_ptr<Object> clone_impl() const override
    {
        return clone_covariant();
    }
};

namespace detail {
    template<class Derived, LayerType lt>
    class BaseLayerProps : public Layer
    {
    public:
        BaseLayerProps(Composition* composition) : Layer(composition, lt) {}


        std::unique_ptr<Derived> clone_covariant() const
        {
            auto object = std::make_unique<Derived>(composition);
            clone_into(object.get());
            return object;
        }

    private:
        std::unique_ptr<Object> clone_impl() const override
        {
            return clone_covariant();
        }
    };
} // namespace detail


class NullLayer : public detail::BaseLayerProps<NullLayer, LayerType::NullLayer>
{};


class ShapeLayer : public detail::BaseLayerProps<ShapeLayer, LayerType::ShapeLayer>
{
    // shapes
};



} // namespace model

Q_DECLARE_METATYPE(model::LayerType)
