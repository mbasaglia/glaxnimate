#pragma once

#include <QMetaType>

#include "model/property.hpp"
#include "model/document_node.hpp"
#include "model/transform.hpp"
#include "utils/range.hpp"

namespace model {


class Composition;
class Layer;

class Layer : public AnimationContainer
{
    Q_OBJECT
    GLAXNIMATE_PROPERTY_REFERENCE(Layer, parent, &Layer::valid_parents, &Layer::is_valid_parent)
    GLAXNIMATE_PROPERTY(float, start_time, 0, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_SUBOBJECT(Transform, transform)

public:
    class ChildLayerIterator
    {
    public:
        using value_type = Layer;
        using reference = value_type&;
        using pointer = value_type*;

        ChildLayerIterator& operator++()
        {
            ++index;
            find_first();
            return *this;
        }

        Layer& operator*() const;
        Layer* operator->() const;

        bool operator==(const ChildLayerIterator& other) const
        {
            return comp == other.comp && parent == other.parent && index == other.index;
        }

        bool operator!=(const ChildLayerIterator& other) const
        {
            return !(*this == other);
        }

    private:
        ChildLayerIterator(const Composition* comp, const Layer* parent, int index)
        : comp(comp),
          parent(parent),
          index(index)
        {
            find_first();
        }

        void find_first();
        friend Layer;
        friend Composition;
        const Composition* comp;
        const Layer* parent;
        int index;
    };

    explicit Layer(Document* doc, Composition* composition);

    utils::Range<ChildLayerIterator> children() const;


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
    graphics::DocumentNodeGraphicsItem* docnode_make_graphics_item() override;
    std::vector<std::unique_ptr<QGraphicsItem>> docnode_make_graphics_editor() override;
    void set_time(FrameTime t) override;


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
    void on_property_changed(const BaseProperty* prop, const QVariant&) override;
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

    std::vector<DocumentNode*> valid_parents() const;
    bool is_valid_parent(DocumentNode* node) const;
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

} // namespace model
