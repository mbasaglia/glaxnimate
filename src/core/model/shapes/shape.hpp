#pragma once

#include "model/document_node.hpp"
#include "math/bezier/bezier.hpp"
#include "model/property/object_list_property.hpp"

namespace model {

using ShapeListProperty = ObjectListProperty<class ShapeElement>;

/**
 * \brief Base class for all shape elements
 */
class ShapeElement : public VisualNode
{
    Q_OBJECT

public:
    using VisualNode::VisualNode;

    DocumentNode* docnode_parent() const override;
    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }

    /**
     * \brief Index within its parent
     */
    int position() const
    {
        return position_;
    }

    virtual void add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const = 0;
    math::bezier::MultiBezier shapes(FrameTime t) const;

    ShapeListProperty* owner() const { return property_; }

    virtual QPainterPath to_clip(FrameTime t) const;
    virtual QPainterPath to_painter_path(FrameTime t) const = 0;
    virtual std::unique_ptr<ShapeElement> to_path() const;

signals:
    void position_updated();
    void siblings_changed();

protected:
    const ShapeListProperty& siblings() const;
    void on_property_changed(const BaseProperty* prop, const QVariant& value) override;

private:
    void set_position(ShapeListProperty* property, int pos);

    ShapeListProperty* property_ = nullptr;
    int position_ = -1;

    friend ShapeListProperty;
};

template<>
class ObjectListProperty<ShapeElement> : public detail::ObjectListProperty<ShapeElement>
{
public:
    using detail::ObjectListProperty<ShapeElement>::ObjectListProperty;

    /**
     * \brief End iterator for a range that includes a modifier then stops
     */
    iterator past_first_modifier() const;

    QRectF bounding_rect(FrameTime t) const;

protected:
    void update_pos(int index)
    {
        int i;
        for ( i = size() - 1; i >= index; i-- )
            objects[i]->set_position(this, i);
        for ( ; i >= 0; i-- )
            objects[i]->siblings_changed();
    }

    void on_insert(int index) override
    {
        update_pos(index);
    }

    void on_remove(int index) override
    {
        update_pos(index);
    }

    void on_move(int index_a, int index_b) override
    {
        if ( index_b < index_a )
            std::swap(index_a, index_b);

        for ( int i = index_a; i <= index_b; i++ )
            objects[i]->set_position(this, i);

        for ( int i = 0; i <= index_b; i++ )
            objects[i]->siblings_changed();
    }
};

class Path;

/**
 * \brief Classes that define shapes on their own (but not necessarily style)
 */
class Shape : public ShapeElement
{
    Q_OBJECT

public:
    using ShapeElement::ShapeElement;

    virtual math::bezier::Bezier to_bezier(FrameTime t) const = 0;

    void add_shapes(FrameTime t, math::bezier::MultiBezier & bez, const QTransform& transform) const override;

    std::unique_ptr<ShapeElement> to_path() const override;
    QPainterPath to_painter_path(FrameTime t) const override;
};

/**
 * \brief Base class for types that perform operations on their sibling shapes
 */
class ShapeOperator : public ShapeElement
{
    Q_OBJECT

public:
    ShapeOperator(model::Document* doc);

    math::bezier::MultiBezier collect_shapes(FrameTime t, const QTransform& transform) const;

    const std::vector<ShapeElement*>& affected() const { return affected_elements; }

protected:
    virtual void do_collect_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const;

private slots:
    void update_affected();

    void sibling_prop_changed(const model::BaseProperty* prop);

signals:
    void shape_changed();

private:
    std::vector<ShapeElement*> affected_elements;
};

/**
 * \brief Base class for elements that modify other shapes
 */
class Modifier : public ShapeOperator
{
    Q_OBJECT

public:
    using ShapeOperator::ShapeOperator;

    void add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const override;

    QPainterPath to_painter_path(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override;

protected:
    virtual math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const = 0;

    /**
     * \brief Wether to process on the whole thing (or individual objects)
     */
    virtual bool process_collected() const = 0;

    void do_collect_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const override;

};

/**
 * \brief CRTP to override some methods using static functions
 * (so said methods can be accessed with no object)
 */
template<class Derived, class Base>
class StaticOverrides : public Base
{
public:
    using Ctor = StaticOverrides;
    using Base::Base;

    QIcon tree_icon() const override
    {
        return Derived::static_tree_icon();
    }

    QString type_name_human() const override
    {
        return Derived::static_type_name_human();
    }

    static QString static_class_name()
    {
        return detail::naked_type_name<Derived>();
    }
};


} // namespace model
