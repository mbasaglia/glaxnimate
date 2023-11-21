/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QIdentityProxyModel>
#include "document_model_base.hpp"

namespace glaxnimate::gui::item_models {

namespace detail {

template<class T>
inline std::enable_if_t<!std::is_same_v<std::decay_t<T>, QModelIndex>, T>
forward(const QAbstractProxyModel*, T v)
{
    return v;
}

template<class T>
inline std::enable_if_t<std::is_same_v<std::decay_t<T>, QModelIndex>, QModelIndex>
forward(const QAbstractProxyModel* proxy, T v)
{
    return proxy->mapFromSource(v);
}


template<class T>
inline std::enable_if_t<!std::is_same_v<std::decay_t<T>, QModelIndex>, T>
reverse(const QAbstractProxyModel*, T v)
{
    return v;
}

template<class T>
inline std::enable_if_t<std::is_same_v<std::decay_t<T>, QModelIndex>, QModelIndex>
reverse(const QAbstractProxyModel* proxy, T v)
{
    return proxy->mapToSource(v);
}

} // namespace detail

class ProxyBase : public QIdentityProxyModel
{
public:
    using QIdentityProxyModel::QIdentityProxyModel;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl<QModelIndex>(&QAbstractItemModel::index, row, column, parent);
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        return forward_impl<QModelIndex>(&QAbstractItemModel::parent, child);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl<int>(&QAbstractItemModel::rowCount, parent);
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl<int>(&QAbstractItemModel::columnCount, parent);
    }

protected:
    // Qt cheats by befriending the proxy models... so we need to cheat too
    DocumentModelBase* friendly_model() const
    {
        return static_cast<DocumentModelBase*>(sourceModel());
    }

    QModelIndex create_source_index(int row, int column, quintptr id) const
    {
        return friendly_model()->createIndex(row, column, id);
    }

    template<class Signal, class Derived, class Ret, class... Args>
    void reconnect(Signal func, Ret (Derived::*slot)(Args...))
    {
        disconnect(sourceModel(), func, this, nullptr);
        connect(sourceModel(), func, static_cast<Derived*>(this), slot);
    }

private:
    template<class Ret, class... Args, class... ActualArgs>
    Ret forward_impl(Ret (QAbstractItemModel::*method)(Args...) const, ActualArgs... args) const
    {
        return detail::forward<Ret>(this, (sourceModel()->*method)(detail::reverse<Args>(this, args)...));
    }
};

} // namespace glaxnimate::gui::item_models
