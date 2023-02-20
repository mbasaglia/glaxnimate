/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "proxy_base.hpp"
#include <QMimeData>

namespace glaxnimate::gui::item_models {

class AssetProxyModel : public ProxyBase
{
public:
    using ProxyBase::ProxyBase;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return qMax(0, sourceModel()->columnCount(mapToSource(parent))) - 1;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if ( is_precomp(parent) )
            return 0;
        return sourceModel()->rowCount(mapToSource(parent));
    }

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override
    {
        return create_source_index(proxyIndex.row(), proxyIndex.column() + 1, proxyIndex.internalId());
    }

    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const override
    {
        return createIndex(sourceIndex.row(), qMax(0, sourceIndex.column()-1), sourceIndex.internalId());
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if ( orientation == Qt::Horizontal )
            section += 1;
        return sourceModel()->headerData(section, orientation, role);
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        return mapFromSource(sourceModel()->index(row, column+1, mapToSource(parent)));
    }

    Qt::ItemFlags flags ( const QModelIndex & index ) const override
    {
        auto flags = sourceModel()->flags(mapToSource(index)) & ~Qt::ItemIsDropEnabled;
        if ( is_precomp(index) )
            flags |= Qt::ItemNeverHasChildren|Qt::ItemIsDragEnabled;
        else if ( cast_index<model::Bitmap>(index) )
            flags |= Qt::ItemNeverHasChildren|Qt::ItemIsDragEnabled;
        return flags;
    }

    QMimeData * mimeData(const QModelIndexList& indexes) const override
    {
        auto data = ProxyBase::mimeData(indexes);
        if ( data )
            data->setData("application/x.glaxnimate-asset-uuid", data->data("application/x.glaxnimate-node-uuid"));
        return data;
    }

protected:
    bool is_precomp(const QModelIndex & index) const
    {
        return cast_index<model::Precomposition>(index);
    }

    template<class T>
    T* cast_index(const QModelIndex & index) const
    {
        return qobject_cast<T*>(static_cast<QObject*>(index.internalPointer()));
    }

    void on_rows_add_begin(const QModelIndex &parent, int first, int last)
    {
        if ( !is_precomp(parent) )
            beginInsertRows(parent, first, last);
    }

    void on_rows_added(const QModelIndex &parent)
    {
        if ( !is_precomp(parent) )
            endInsertRows();
    }

    void source_changed(QAbstractItemModel * source_model)
    {
        if ( source_model )
        {
            reconnect(&QAbstractItemModel::rowsAboutToBeInserted, &AssetProxyModel::on_rows_add_begin);
            reconnect(&QAbstractItemModel::rowsInserted, &AssetProxyModel::on_rows_added);
        }
    }
};

} // namespace glaxnimate::gui::item_models
