#pragma once

#include <QAbstractItemModel>

#include "model/document.hpp"

namespace item_models {

class PropertyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Columns
    {
        ColumnName,
        ColumnValue,
        ColumnColor,
        ColumnVisible,
        ColumnLocked,

        ColumnCount

    };

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

    PropertyModel(bool animation_only=false);
    ~PropertyModel();

    QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    QModelIndex parent(const QModelIndex & child) const override;

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    int columnCount(const QModelIndex & parent) const override;
    int rowCount(const QModelIndex & parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void set_document(model::Document* document);

    void clear_document()
    {
        set_document(nullptr);
    }

    void clear_objects();
    void set_object(model::Object* object);
    void add_object(model::Object* object);
    void add_object_without_properties(model::Object* object);

    Item item(const QModelIndex& index) const;

    QModelIndex property_index(model::BaseProperty* anim) const;
    QModelIndex object_index(model::Object* obj) const;

    model::VisualNode* visual_node(const QModelIndex& index) const;

private slots:
    void property_changed(const model::BaseProperty* prop, const QVariant& value);
    void on_delete_object();

signals:
    /**
     * \brief Emitted on set_document
     */
    void document_changed(model::Document* document);
    /**
     * \brief Emitted when the model starts adding a root object, before any properties are added
     */
    void root_object_added_begin(model::Object* object);
    /**
     * \brief Emitted when a property is added, the property belongs to the last
     * root_object_added_begin() argument or one of its sub-objects
     */
    void property_added(model::BaseProperty* property);
    /**
     * \brief Emitted when a root object is being removed from the model
     */
    void object_removed(model::Object* object);
    /**
     * \brief Called when all objects are removed
     */
    void objects_cleared();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace item_models
