#pragma once

#include "proxy_base.hpp"

namespace item_models {

class AssetProxyModel : public ProxyBase<AssetProxyModel>
{
private:
    friend Ctor;
    void source_changed(QAbstractItemModel *)
    {}

public:
    using Ctor::Ctor;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return qMax(0, sourceModel()->columnCount(mapToSource(parent))) - 1;
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
};

} // namespace item_models
