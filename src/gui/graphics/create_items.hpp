#pragma once

#include <vector>
#include <memory>

#include <QGraphicsItem>


namespace model {
    class DocumentNode;
} // namespace model

namespace graphics {

class DocumentNodeGraphicsItem;
class GraphicsEditor;

class GraphicsItemFactory
{
public:
    static GraphicsItemFactory& instance()
    {
        static GraphicsItemFactory instance;
        return instance;
    }

    DocumentNodeGraphicsItem* make_graphics_item(model::DocumentNode* node) const;
    std::unique_ptr<GraphicsEditor> make_graphics_editor(model::DocumentNode* node) const;


private:
    class AbstractBuilder
    {
    public:
        virtual ~AbstractBuilder() = default;
        virtual DocumentNodeGraphicsItem* make_graphics_item(model::DocumentNode* node) const = 0;
        virtual std::unique_ptr<GraphicsEditor> make_graphics_editor(model::DocumentNode* node) const = 0;
    };

    template<class DocT, class FuncItem, class FuncEdit>
    class Builder : public graphics::GraphicsItemFactory::AbstractBuilder
    {
    public:

        Builder(FuncItem func_item, FuncEdit func_edit)
            : func_item(std::move(func_item)),
              func_edit(std::move(func_edit))
        {}

        graphics::DocumentNodeGraphicsItem* make_graphics_item(model::DocumentNode* node) const override
        {
            return func_item(static_cast<DocT*>(node));
        }

        std::unique_ptr<GraphicsEditor> make_graphics_editor(model::DocumentNode* node) const override
        {
            return func_edit(static_cast<DocT*>(node));
        }

        FuncItem func_item;
        FuncEdit func_edit;
    };

    GraphicsItemFactory();
    GraphicsItemFactory(const GraphicsItemFactory&) = delete;
    GraphicsItemFactory(GraphicsItemFactory&&) = delete;
    ~GraphicsItemFactory() = default;

    template<class DocT, class FuncItem, class FuncEdit>
    void register_builder(FuncItem func_item, FuncEdit func_edit)
    {
        builders[&DocT::staticMetaObject] = new Builder<DocT, FuncItem, FuncEdit>(
            std::move(func_item), std::move(func_edit)
        );
    }

    static graphics::DocumentNodeGraphicsItem* make_graphics_item_default(model::DocumentNode* node);
    static std::unique_ptr<GraphicsEditor> make_graphics_editor_default(model::DocumentNode* node);

    AbstractBuilder* builder_for(model::DocumentNode* node) const;

    std::map<const QMetaObject*, AbstractBuilder*> builders;
};

} // namespace graphics
