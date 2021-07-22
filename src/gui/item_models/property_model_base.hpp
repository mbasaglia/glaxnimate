#pragma once

#include "document_model_base.hpp"

namespace glaxnimate::gui::item_models {

class PropertyModelBase : public DocumentModelBase
{
    Q_OBJECT

public:
    enum CustomData
    {
        ReferenceProperty = Qt::UserRole,
        MinValue,
        MaxValue,
        Flags
    };

    struct Item
    {
        constexpr Item() noexcept = default;
        constexpr Item(model::Object* object, model::BaseProperty* property = nullptr) noexcept :
            object(object),
            property(property)
        {}

        explicit constexpr operator bool() const noexcept
        {
            return object;
        }

        model::Object* object = nullptr;
        model::BaseProperty* property = nullptr;
    };

    PropertyModelBase();
    ~PropertyModelBase();

    QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    QModelIndex parent(const QModelIndex & child) const override;

    void set_document(model::Document* document);

    void clear_document();

    int rowCount(const QModelIndex & parent) const override;


    Item item(const QModelIndex& index) const;

    QModelIndex property_index(model::BaseProperty* anim) const;
    QModelIndex object_index(model::Object* obj) const;
    QModelIndex index_by_id(quintptr id, int column = 0) const;

    model::VisualNode* visual_node(const QModelIndex& index) const override;
    model::DocumentNode* node(const QModelIndex& index) const override;
    QModelIndex node_index(model::DocumentNode* node) const override;
    model::Document* document() const override;

private slots:
    void property_changed(const model::BaseProperty* prop, const QVariant& value);
    void on_delete_object();

protected:
    virtual void on_document_reset() = 0;

protected:
    class Private;
    PropertyModelBase(std::unique_ptr<Private>(d));

    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::item_models

