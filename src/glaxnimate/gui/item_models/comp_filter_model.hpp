#pragma once

#include "proxy_base.hpp"

namespace glaxnimate::gui::item_models {

class CompFilterModel : public ProxyBase
{
public:
    using ProxyBase::ProxyBase;

    void set_composition(model::Composition* comp)
    {
        if ( !sourceModel() )
            return;

        set_root(friendly_model()->node_index(comp));
    }

    quintptr get_root_id() const
    {
        return root_id;
    }

    void set_root(QModelIndex root)
    {
        beginResetModel();
        root_id = root.internalId();
        this->root = root;
        endResetModel();
    }

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override
    {
        if ( !root.isValid() || !proxyIndex.isValid() )
            return {};

        int row = proxyIndex.row();
        if ( proxyIndex.internalId() == root_id )
            row = root.row();

        return create_source_index(row, proxyIndex.column(), proxyIndex.internalId());
    }

    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const override
    {
        if ( !root.isValid() || !sourceIndex.isValid() )
            return {};

        int row = sourceIndex.row();
        if ( sourceIndex.internalId() == root_id )
            row = 0;

        return createIndex(row, sourceIndex.column(), sourceIndex.internalId());
    }

    QModelIndex parent ( const QModelIndex & child ) const override
    {
        if ( !root.isValid() )
            return {};
        if ( child.internalId() == root_id )
            return {};
        return mapFromSource(mapToSource(child).parent());
    }

    int rowCount ( const QModelIndex & parent ) const override
    {
        if ( !root.isValid() )
            return 0;

        if ( !parent.isValid() )
            return 1;

        return sourceModel()->rowCount(mapToSource(parent));
    }

    int columnCount(const QModelIndex & parent) const override
    {
        return sourceModel()->columnCount(mapToSource(parent));
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if ( !root.isValid() )
            return {};

        if ( !parent.isValid() )
        {
            if ( row == 0 )
                return mapFromSource(root.sibling(root.row(), column));
            return {};
        }

        return mapFromSource(sourceModel()->index(row, column, mapToSource(parent)));
    }

private:
    bool has_been_removed()
    {
        if ( root_id && !root.isValid() )
        {
            set_root({});
            return true;
        }
        return false;
    }

    void on_rows_removed_begin(const QModelIndex& index, int from, int to)
    {
        if ( !root_id || root.parent() != index )
            beginRemoveRows(mapFromSource(index), from ,to);
    }

    void on_rows_removed()
    {
        if ( !has_been_removed() )
            endRemoveRows();
    }

    void on_cols_removed_begin(const QModelIndex& index, int from, int to)
    {
        if ( !root_id || root.parent() != index )
            beginRemoveColumns(mapFromSource(index), from ,to);
    }

    void on_cols_removed()
    {
        if ( !has_been_removed() )
            endRemoveColumns();
    }

protected:
    void source_changed(QAbstractItemModel * source_model)
    {
        root_id = 0;
        root = QModelIndex();
        if ( source_model )
        {
            reconnect(&QAbstractItemModel::rowsAboutToBeRemoved, &CompFilterModel::on_rows_removed_begin);
            reconnect(&QAbstractItemModel::rowsRemoved, &CompFilterModel::on_rows_removed);

            reconnect(&QAbstractItemModel::columnsRemoved, &CompFilterModel::on_cols_removed);
            reconnect(&QAbstractItemModel::columnsAboutToBeRemoved, &CompFilterModel::on_cols_removed_begin);
        }
    }

private:
    quintptr root_id = 0;
    QPersistentModelIndex root;
};

} // namespace glaxnimate::gui::item_models
