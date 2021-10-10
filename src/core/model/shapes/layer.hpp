#pragma once

#include "group.hpp"
#include "model/property/reference_property.hpp"
#include "model/animation_container.hpp"
#include "model/mask_settings.hpp"

namespace glaxnimate::model {

class Layer : public StaticOverrides<Layer, Group>
{
    GLAXNIMATE_OBJECT(Layer)
    GLAXNIMATE_SUBOBJECT(AnimationContainer, animation)
    GLAXNIMATE_PROPERTY_REFERENCE(Layer, parent, &Layer::valid_parents, &Layer::is_valid_parent, &Layer::docnode_on_update_group)
    /**
     * \brief Whether the layer will be rendered / exported in other formats
     */
    GLAXNIMATE_PROPERTY(bool, render, true)
    GLAXNIMATE_SUBOBJECT(MaskSettings, mask)

public:
    class ChildLayerIterator
    {
    public:
        using value_type = VisualNode;
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

    VisualNode* docnode_group_parent() const override;
    int docnode_group_child_count() const override;
    VisualNode* docnode_group_child(int index) const override;
    QIcon tree_icon() const override;
    static QIcon static_tree_icon();
    static QString static_type_name_human() { return tr("Layer"); }
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

    void paint(QPainter*, FrameTime, PaintMode, model::Modifier* modifier) const override;

    QPainterPath to_clip(model::FrameTime t) const override;
    QPainterPath to_painter_path(model::FrameTime t) const override;

    std::unique_ptr<ShapeElement> to_path() const override;

    bool is_valid_parent(DocumentNode* node) const;

private:
    std::vector<DocumentNode*> valid_parents() const;
};

} // namespace glaxnimate::model
