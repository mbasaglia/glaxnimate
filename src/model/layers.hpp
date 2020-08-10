#pragma once

#include <QMetaType>
#include <QColor>

#include "property.hpp"
#include "document_node.hpp"
#include "transform.hpp"

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



class Layer : public AnimationContainer
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY_REFERENCE(Layer, parent)
    GLAXNIMATE_PROPERTY(float, start_time, 0)
    GLAXNIMATE_SUBOBJECT(Transform, transform)

public:
    explicit Layer(Document* doc, Composition* composition);

    ChildLayerView children() const
    {
        return ChildLayerView(composition_, this);
    }


    std::unique_ptr<Layer> clone_covariant() const
    {
        auto object = std::make_unique<Layer>(document(), composition_);
        clone_into(object.get());
        return object;
    }

    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_count() const override { return 0; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    QIcon docnode_icon() const override { return QIcon::fromTheme("folder"); }
    QString type_name_human() const override { return tr("Uknown Layer"); }
    DocumentNode* docnode_parent() const override;
    DocumentNode* docnode_group_parent() const override;
    std::vector<DocumentNode*> docnode_valid_references(const ReferencePropertyBase*) const override;
    bool docnode_is_valid_reference(const ReferencePropertyBase* property, DocumentNode* node) const override;
    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;
    std::vector<std::unique_ptr<QGraphicsItem>> docnode_make_graphics_editor() override;
    

    QRectF local_bounding_rect(FrameTime t) const override;
//     QRectF bounding_rect(FrameTime t) const override;
//     QPolygonF unaligned_bounding_rect(FrameTime t) const override;

    /**
     * \brief Returns the (frame) time relative to this layer
     *
     * Useful for stretching / remapping etc.
     * Always use this to get animated property values,
     * even if currently it doesn't do anything
     */
    double relative_time(double time) const { return time; }

    bool is_ancestor_of(const Layer* other) const;

    Composition* composition() const { return composition_; }
    void set_composition(Composition* composition);

    QTransform transform_matrix() const;
    QTransform transform_matrix(FrameTime t) const;

signals:
    void transform_matrix_changed(const QTransform& t);

protected:
    void on_property_changed(const QString& name, const QVariant&) override;
    void on_paint(QPainter*, FrameTime, PaintMode) const override;
    virtual void on_paint_untransformed(QPainter*, FrameTime) const {}

private slots:
    void on_transform_matrix_changed();

private:
    Composition* composition_;

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
            auto object = std::make_unique<Derived>(document(), composition());
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


class SolidColorLayer : public detail::BaseLayerProps<SolidColorLayer>
{
    Q_OBJECT

    //                  type    name    default     edit    notify                                          validate
    GLAXNIMATE_PROPERTY(float,  width,  0,          true,   &SolidColorLayer::bounding_rect_changed, &SolidColorLayer::positive)
    GLAXNIMATE_PROPERTY(float,  height, 0,          true,   &SolidColorLayer::bounding_rect_changed, &SolidColorLayer::positive)
    GLAXNIMATE_PROPERTY(QColor, color,  Qt::white)
public:
    SolidColorLayer(Document* doc, Composition* composition);

    QRectF local_bounding_rect(FrameTime t) const override;

    QIcon docnode_icon() const override
    {
        return QIcon::fromTheme("object-fill");
    }

    QString type_name_human() const override { return tr("Solid Color Layer"); }
    
    bool docnode_selection_container() const override { return false; }

protected:
    void on_paint_untransformed(QPainter*, FrameTime) const override;

private:
    bool positive(float x) const
    {
        return x > 0;
    }
};


} // namespace model
