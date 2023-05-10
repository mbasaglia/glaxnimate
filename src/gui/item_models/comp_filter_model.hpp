/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "proxy_base.hpp"
#include "model/assets/composition.hpp"

namespace glaxnimate::gui::item_models {

class CompFilterModel : public ProxyBase
{
    Q_OBJECT

public:
    using ProxyBase::ProxyBase;

    void set_composition(model::Composition* comp);

    model::Composition* composition() const;

    quintptr get_root_id() const;

    void set_root(QModelIndex root);

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const override;
    QModelIndex parent ( const QModelIndex & child ) const override;
    int rowCount ( const QModelIndex & parent ) const override;
    int columnCount(const QModelIndex & parent) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

signals:
    void composition_changed(model::Composition* comp);

private:
    bool has_been_removed();

    void on_rows_removed_begin(const QModelIndex& index, int from, int to);

    void on_rows_removed();

    void on_cols_removed_begin(const QModelIndex& index, int from, int to);

    void on_cols_removed();

protected:
    void source_changed(QAbstractItemModel * source_model);

private:
    quintptr root_id = 0;
    QPersistentModelIndex root;
    model::Composition* comp = nullptr;
};

} // namespace glaxnimate::gui::item_models
