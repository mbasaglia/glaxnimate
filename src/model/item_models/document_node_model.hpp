#pragma once


#include <QAbstractItemModel>

#include "model/document.hpp"

namespace model {


class DocumentNodeModel : public QAbstractItemModel
{
public:
    enum ColumnTypes
    {
        ColumnColor = 0,
        ColumnName,
        ColumnVisible,
        ColumnLocked,

        ColumnCount
    };

    int rowCount ( const QModelIndex & parent ) const override;
    int columnCount ( const QModelIndex & parent ) const override;
    bool moveRows ( const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild ) override;
    bool removeRows ( int row, int count, const QModelIndex & parent ) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data ( const QModelIndex & index, int role ) const override;
    Qt::ItemFlags flags ( const QModelIndex & index ) const override;
    QModelIndex parent ( const QModelIndex & child ) const override;
    bool setData ( const QModelIndex & index, const QVariant & value, int role ) override;


    void clear();
    void set_document(model::Document* doc);
    model::DocumentNode* node(const QModelIndex& index) const;

private:
    model::Document* document = nullptr;
};


} // namespace model
