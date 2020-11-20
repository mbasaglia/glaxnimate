#pragma once

#include <QAbstractItemModel>

#include "model/document.hpp"

namespace item_models {

class PropertyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum CustomData
    {
        ReferenceProperty = Qt::UserRole,
        MinValue,
        MaxValue
    };

    struct Item
    {
        constexpr Item() noexcept = default;
        constexpr Item(model::Object* object) noexcept : object(object) {}
        constexpr Item(model::AnimatableBase* animatable) noexcept : animatable(animatable) {}

        explicit constexpr operator bool() const noexcept
        {
            return object || animatable;
        }

        model::Object* object = nullptr;
        model::AnimatableBase* animatable = nullptr;
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

private slots:
    void property_changed(const model::BaseProperty* prop, const QVariant& value);
    void on_delete_object();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace item_models
