#pragma once

#include <QMetaType>
#include <QColor>

#include "property.hpp"
#include "document_node.hpp"

namespace model {


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



class Layer : public DocumentNode
{
    Q_OBJECT

public:
    explicit Layer(Document* doc, Composition* composition)
        : DocumentNode(doc), composition(composition)
    {}

    Composition* composition;

    Property<Layer*> parent{this, "parent", nullptr};
    Property<float> in_point{this, "in_point", 0};
    Property<float> out_point{this, "out_point", 0};
    Property<float> start_time{this, "start_time", 0};

    ChildLayerView children() const
    {
        return ChildLayerView(composition, this);
    }


    std::unique_ptr<Layer> clone_covariant() const
    {
        auto object = std::make_unique<Layer>(document(), composition);
        clone_into(object.get());
        return object;
    }

    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_count() const override { return 0; }

    DocumentNode* docnode_parent() const override;

    QIcon docnode_icon() const override { return QIcon::fromTheme("folder"); }

    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override
    {
        return nullptr;
    }

    QString type_name_human() const override { return tr("Uknown Layer"); }

private:
    std::unique_ptr<Object> clone_impl() const override
    {
        return clone_covariant();
    }
};

namespace detail {
    template<class Derived>
    class BaseLayerProps : public Layer
    {
    public:
        BaseLayerProps(Document* doc, Composition* composition) : Layer(doc, composition) {}


        std::unique_ptr<Derived> clone_covariant() const
        {
            auto object = std::make_unique<Derived>(document(), composition);
            clone_into(object.get());
            return object;
        }

    protected:
        using Ctor = BaseLayerProps;

    private:
        std::unique_ptr<Object> clone_impl() const override
        {
            return clone_covariant();
        }
    };
} // namespace detail


class EmptyLayer : public detail::BaseLayerProps<EmptyLayer>
{
    Q_OBJECT

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("transform-move");
    }

    QString type_name_human() const override { return tr("Empty Layer"); }
};


class ShapeLayer : public detail::BaseLayerProps<ShapeLayer>
{
    Q_OBJECT

public:
    // shapes

    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("shapes");
    }

    QString type_name_human() const override { return tr("Shape Layer"); }
};



} // namespace model
