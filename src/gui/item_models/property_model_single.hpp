#pragma once

#include "property_model_base.hpp"

namespace item_models {

class PropertyModelSingle : public PropertyModelBase
{
    Q_OBJECT

public:
    enum Columns
    {
        ColumnName,
        ColumnValue,

        ColumnCount

    };

    PropertyModelSingle();

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void clear_objects();
    void set_object(model::Object* object);
//     void add_object(model::Object* object);
//     void add_object_without_properties(model::Object* object);

protected:
    void on_document_reset() override {}
    std::pair<model::VisualNode *, int> drop_position(const QModelIndex & parent, int row) const override;

private:
    class Private;
    Private* dd() const;
};

} // namespace item_models
