#pragma once

#include "model/document_node.hpp"
#include "math/bezier.hpp"

namespace model {

class ShapeListProperty;

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


protected:
    const ShapeListProperty& siblings() const;

private:
    void set_position(ShapeListProperty* property, int pos)
    {
        property_ = property;
        position_ = pos;
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

protected:
    void on_insert(int index) override
    {
        objects[index]->set_position(this, index);
    }

    void on_remove(int index) override
    {
        for ( int i = index; i < size(); i++ )
            objects[i]->set_position(this, i);
    }

    void on_swap(int index_a, int index_b) override
    {
        objects[index_a]->set_position(this, index_a);
        objects[index_b]->set_position(this, index_b);
    }
};

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

class Modifier : public ShapeElement
{
    Q_OBJECT

public:
    using ShapeElement::ShapeElement;

    void add_shapes(FrameTime t, math::MultiBezier& bez) const override
    {
        collect_shapes(t, bez);
    }

protected:
    void collect_shapes(FrameTime t, math::MultiBezier& bez) const
    {
        const ShapeListProperty& prop = siblings();
        for ( auto it = prop.begin() + position() + 1; it != prop.end(); ++it )
        {
            (*it)->add_shapes(t, bez);
            if ( qobject_cast<Modifier*>(it->get()) )
                break;
        }
    }

    math::MultiBezier collect_shapes(FrameTime t) const
    {
        math::MultiBezier bez;
        collect_shapes(t, bez);
        return bez;
    }
//     class AffectedRange
//     {
//     public:
//         AffectedRange(const Modifier* item)
//         : list(&item->siblings()),
//         position(item->position())
//         {}
//
//         ShapeListProperty::iterator begin() const
//         {
//             return list->begin() + position;
//         }
//
//         ShapeListProperty::iterator end() const
//         {
//             return list->end();
//         }
//
//     private:
//         const ShapeListProperty* list;
//         int position;
//     };
};

} // namespace model
