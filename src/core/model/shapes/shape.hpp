#pragma once

#include "model/document_node.hpp"
#include "math/bezier.hpp"
#include "model/property/object_list_property.hpp"

namespace model {

class ShapeListProperty;

/**
 * \brief Base class for all shape elements
 */
class ShapeElement : public DocumentNode
{
    Q_OBJECT

public:
    using DocumentNode::DocumentNode;

    DocumentNode* docnode_parent() const override;
    int docnode_child_count() const override { return 0; }
    DocumentNode* docnode_child(int) const override { return nullptr; }
    int docnode_child_index(DocumentNode*) const override { return -1; }
    bool docnode_selection_container() const override { return false; }

    /**
     * \brief Index within its parent
     */
    int position() const
    {
        return position_;
    }

    virtual void add_shapes(FrameTime t, math::MultiBezier& bez) const = 0;
    math::MultiBezier shapes(FrameTime t) const
    {
        math::MultiBezier bez;
        add_shapes(t, bez);
        return bez;
    }

    ShapeListProperty* owner() const { return property_; }

signals:
    void position_updated();
    void siblings_changed();

protected:
    const ShapeListProperty& siblings() const;

private:
    void set_position(ShapeListProperty* property, int pos)
    {
        property_ = property;
        position_ = pos;
        position_updated();
    }

    ShapeListProperty* property_ = nullptr;
    int position_ = -1;

    friend class ShapeListProperty;
};

class ShapeListProperty : public ObjectListProperty<ShapeElement>
{
public:
    using ObjectListProperty<ShapeElement>::ObjectListProperty;

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

/**
 * \brief Classes that define shapes on their own (but not necessarily style)
 */
class Shape : public ShapeElement
{
    Q_OBJECT

public:
    using ShapeElement::ShapeElement;

    virtual math::Bezier to_bezier(FrameTime t) const = 0;

    void add_shapes(FrameTime t, math::MultiBezier & bez) const override
    {
        bez.beziers().push_back(to_bezier(t));
    }
};

/**
 * \brief Base class for types that perform operations on their sibling shapes
 */
class ShapeOperator : public ShapeElement
{
    Q_OBJECT

public:
    ShapeOperator(model::Document* doc);

    math::MultiBezier collect_shapes(FrameTime t) const
    {
        math::MultiBezier bez;
        collect_shapes(t, bez);
        return bez;
    }

protected:
    void collect_shapes(FrameTime t, math::MultiBezier& bez) const;

//     const std::vector<ShapeElement*>& affected() const { return affected_elements; }

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

    void add_shapes(FrameTime t, math::MultiBezier& bez) const override
    {
        bez.append(process(collect_shapes(t)));
    }

protected:
    virtual math::MultiBezier process(const math::MultiBezier& mbez) const = 0;
};

/**
 * \brief Base class for elements that add a style
 */
class Styler : public ShapeOperator
{
    Q_OBJECT

public:
    using ShapeOperator::ShapeOperator;

    void add_shapes(FrameTime, math::MultiBezier&) const override {}
};

} // namespace model
