#pragma once

#include "group.hpp"
#include "model/property/reference_property.hpp"

namespace model {

class Layer__new : public ObjectBase<Layer__new, Group>
{
    using Layer = Layer__new;
    GLAXNIMATE_OBJECT
    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)
    GLAXNIMATE_PROPERTY_REFERENCE(Layer, parent, &Layer::valid_parents, &Layer::is_valid_parent, &Layer__new::docnode_on_update_group)
    GLAXNIMATE_PROPERTY(float, start_time, 0, {}, {}, PropertyTraits::Visual)


public:
    class ChildLayerIterator
    {
    public:
        using value_type = DocumentNode;
        using reference = value_type&;
        using pointer = value_type*;
        using difference_type = int;
        using iterator_category = std::forward_iterator_tag;

        ChildLayerIterator& operator++()
        {
            ++index;
            find_first();
            return *this;
        }

        pointer operator*() const;
        pointer operator->() const;

        bool operator==(const ChildLayerIterator& other) const
        {
            return comp == other.comp && parent == other.parent && index == other.index;
        }

        bool operator!=(const ChildLayerIterator& other) const
        {
            return !(*this == other);
        }

    private:
        ChildLayerIterator(const ShapeListProperty* comp, const Layer* parent, int index)
        : comp(comp),
          parent(parent),
          index(index)
        {
            find_first();
        }

        void find_first();
        friend Layer;
        friend Composition;
        const ShapeListProperty* comp;
        const Layer* parent;
        int index;
    };

    using Ctor::Ctor;

    DocumentNode* docnode_group_parent() const override;
    int docnode_group_child_count() const override;
    DocumentNode* docnode_group_child(int index) const override;
    QIcon docnode_icon() const override { return QIcon::fromTheme("folder"); }
    QString type_name_human() const override { return tr("Layer"); }
    void set_time(FrameTime t) override;

    /**
     * \brief Returns the (frame) time relative to this layer
     *
     * Useful for stretching / remapping etc.
     * Always use this to get animated property values,
     * even if currently it doesn't do anything
     */
    FrameTime relative_time(FrameTime time) const { return time; }

    bool is_ancestor_of(const Layer* other) const;

    bool is_top_level() const;

private:
    std::vector<ReferenceTarget*> valid_parents() const;
    bool is_valid_parent(ReferenceTarget* node) const;
};

} // namespace model
