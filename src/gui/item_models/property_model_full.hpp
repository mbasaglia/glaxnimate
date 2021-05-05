#pragma once

#include "property_model_base.hpp"

namespace item_models {

class PropertyModelFull : public PropertyModelBase
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

    PropertyModelFull();

    bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    QVariant data(const QModelIndex & index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

protected:
    void on_document_reset() override;

private:
    class Private;
    Private* dd() const;
};

} // namespace item_models

