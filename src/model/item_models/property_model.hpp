#pragma once

#include <QAbstractItemModel>

#include "model/property.hpp"
#include "model/document.hpp"

namespace model {

class PropertyModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    PropertyModel();
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

    void set_object(model::Object* object);

    void clear_object()
    {
        set_object(nullptr);
    }

private slots:
    void property_changed(const QString& name, const QVariant& value);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
