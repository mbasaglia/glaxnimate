/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "comp_filter_model.hpp"


void glaxnimate::gui::item_models::CompFilterModel::set_composition(model::Composition* comp)
{
    if ( !sourceModel() )
        return;

    this->comp = comp;
    set_root(friendly_model()->node_index(comp));
    emit composition_changed(comp);
}

glaxnimate::model::Composition* glaxnimate::gui::item_models::CompFilterModel::composition() const
{
    return comp;
}

quintptr glaxnimate::gui::item_models::CompFilterModel::get_root_id() const
{
    return root_id;
}

void glaxnimate::gui::item_models::CompFilterModel::set_root(QModelIndex root)
{
    beginResetModel();
    root_id = root.internalId();
    this->root = root;
    endResetModel();
}

QModelIndex glaxnimate::gui::item_models::CompFilterModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if ( !root.isValid() || !proxyIndex.isValid() )
        return {};

    int row = proxyIndex.row();
    if ( proxyIndex.internalId() == root_id )
        row = root.row();

    return create_source_index(row, proxyIndex.column(), proxyIndex.internalId());
}

QModelIndex glaxnimate::gui::item_models::CompFilterModel::mapFromSource(const QModelIndex & sourceIndex) const
{
    if ( !root.isValid() || !sourceIndex.isValid() )
        return {};

    int row = sourceIndex.row();
    if ( sourceIndex.internalId() == root_id )
        row = 0;

    return createIndex(row, sourceIndex.column(), sourceIndex.internalId());
}

QModelIndex glaxnimate::gui::item_models::CompFilterModel::parent ( const QModelIndex & child ) const
{
    if ( !root.isValid() )
        return {};
    if ( child.internalId() == root_id )
        return {};
    return mapFromSource(mapToSource(child).parent());
}

int glaxnimate::gui::item_models::CompFilterModel::rowCount ( const QModelIndex & parent ) const
{
    if ( !root.isValid() )
        return 0;

    if ( !parent.isValid() )
        return 1;

    return sourceModel()->rowCount(mapToSource(parent));
}

int glaxnimate::gui::item_models::CompFilterModel::columnCount(const QModelIndex & parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

QModelIndex glaxnimate::gui::item_models::CompFilterModel::index(int row, int column, const QModelIndex &parent) const
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

bool glaxnimate::gui::item_models::CompFilterModel::has_been_removed()
{
    if ( root_id && !root.isValid() )
    {
        set_root({});
        return true;
    }
    return false;
}

void glaxnimate::gui::item_models::CompFilterModel::on_rows_removed_begin(const QModelIndex& index, int from, int to)
{
    if ( !root_id || root.parent() != index )
        beginRemoveRows(mapFromSource(index), from ,to);
}

void glaxnimate::gui::item_models::CompFilterModel::on_rows_removed()
{
    if ( !has_been_removed() )
        endRemoveRows();
}

void glaxnimate::gui::item_models::CompFilterModel::on_cols_removed_begin(const QModelIndex& index, int from, int to)
{
    if ( !root_id || root.parent() != index )
        beginRemoveColumns(mapFromSource(index), from ,to);
}

void glaxnimate::gui::item_models::CompFilterModel::on_cols_removed()
{
    if ( !has_been_removed() )
        endRemoveColumns();
}

void glaxnimate::gui::item_models::CompFilterModel::source_changed(QAbstractItemModel * source_model)
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
