#pragma once

#include "model/document_node.hpp"
#include "math/bezier/bezier.hpp"
#include "model/property/object_list_property.hpp"

namespace glaxnimate::model {

using ShapeListProperty = ObjectListProperty<class ShapeElement>;
class Composition;

/**
 * \brief Base class for all shape elements
 */
class ShapeElement : public VisualNode
{
    Q_OBJECT

public:
    explicit ShapeElement(model::Document* document);
    ~ShapeElement();

    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }

    /**
     * \brief Index within its parent
     */
    int position() const;

    virtual void add_shapes(FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const = 0;
    math::bezier::MultiBezier shapes(FrameTime t) const;

    ShapeListProperty* owner() const;
    Composition* owner_composition() const;
    void clear_owner();

    virtual QPainterPath to_clip(FrameTime t) const;
    virtual QPainterPath to_painter_path(FrameTime t) const = 0;
    virtual std::unique_ptr<ShapeElement> to_path() const;

signals:
    void position_updated();
    void siblings_changed();

protected:
    const ShapeListProperty& siblings() const;
    void on_property_changed(const BaseProperty* prop, const QVariant& value) override;
    void on_parent_changed(model::DocumentNode* old_parent, model::DocumentNode* new_parent) override;
    virtual void on_composition_changed(model::Composition* old_comp, model::Composition* new_comp)
    {
        Q_UNUSED(old_comp);
        Q_UNUSED(new_comp);
    }

private:
    void set_position(ShapeListProperty* property, int pos);

    class Private;
    std::unique_ptr<Private> d;
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

    GLAXNIMATE_PROPERTY(bool, reversed, false, {}, {}, PropertyTraits::Visual|PropertyTraits::Hidden)

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
    math::bezier::MultiBezier collect_shapes_from(const std::vector<ShapeElement*>& shapes, FrameTime t, const QTransform& transform) const;

    const std::vector<ShapeElement*>& affected() const { return affected_elements; }

protected:
    virtual void do_collect_shapes(const std::vector<ShapeElement*>& shapes, FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const;
    virtual bool skip_stylers() const { return true; }

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

    virtual math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const = 0;

protected:
    /**
     * \brief Whether to process on the whole thing (or individual objects)
     */
    virtual bool process_collected() const = 0;

    void do_collect_shapes(const std::vector<ShapeElement*>& shapes, FrameTime t, math::bezier::MultiBezier& bez, const QTransform& transform) const override;

    virtual bool skip_stylers() const override { return false; }

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


} // namespace glaxnimate::model
