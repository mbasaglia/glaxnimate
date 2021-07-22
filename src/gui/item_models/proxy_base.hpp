#pragma once
#include <QAbstractProxyModel>
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

template<class... Args>
class SignalForwarder
{
public:
    using Functor = void (QAbstractItemModel::*)(Args...);
    QAbstractProxyModel* proxy;
    Functor signal;

    SignalForwarder(QAbstractProxyModel* proxy, Functor signal)
        : proxy(proxy), signal(signal)
    {}

    inline void operator()(Args... args) const
    {
        (proxy->*signal)(forward<Args>(proxy, args)...);
    }
};
} // namespace detail

class ProxyBase : public QAbstractProxyModel
{
public:
    using QAbstractProxyModel::QAbstractProxyModel;

    void setSourceModel(QAbstractItemModel *new_model) override
    {
        beginResetModel();

        if ( sourceModel() )
            QObject::disconnect(sourceModel(), nullptr, this, nullptr);

        QAbstractProxyModel::setSourceModel(new_model);


        if ( sourceModel() )
        {
            forward_signal(&QAbstractItemModel::columnsAboutToBeInserted);
            forward_signal(&QAbstractItemModel::columnsAboutToBeMoved);
            forward_signal(&QAbstractItemModel::columnsAboutToBeRemoved);
            forward_signal(&QAbstractItemModel::columnsInserted);
            forward_signal(&QAbstractItemModel::columnsMoved);
            forward_signal(&QAbstractItemModel::columnsRemoved);
            forward_signal(&QAbstractItemModel::dataChanged);
            forward_signal(&QAbstractItemModel::headerDataChanged);
            forward_signal(&QAbstractItemModel::layoutAboutToBeChanged);
            forward_signal(&QAbstractItemModel::layoutChanged);
            forward_signal(&QAbstractItemModel::modelAboutToBeReset);
            forward_signal(&QAbstractItemModel::modelReset);
            forward_signal(&QAbstractItemModel::rowsAboutToBeInserted);
            forward_signal(&QAbstractItemModel::rowsAboutToBeMoved);
            forward_signal(&QAbstractItemModel::rowsAboutToBeRemoved);
            forward_signal(&QAbstractItemModel::rowsInserted);
            forward_signal(&QAbstractItemModel::rowsMoved);
            forward_signal(&QAbstractItemModel::rowsRemoved);
        }

        on_source_changed(new_model);

        endResetModel();
    }


    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl(&QAbstractItemModel::index, row, column, parent);
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        return forward_impl(&QAbstractItemModel::parent, child);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl(&QAbstractItemModel::rowCount, parent);
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return forward_impl(&QAbstractItemModel::columnCount, parent);
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

    virtual void on_source_changed(QAbstractItemModel *) {};

private:
    template<class... Args>
    void forward_signal(void (QAbstractItemModel::*signal)(Args...))
    {
        QObject::connect(sourceModel(), signal, this, detail::SignalForwarder(this, signal));

    }

    template<class Ret, class... Args, class... ActualArgs>
    Ret forward_impl(Ret (QAbstractItemModel::*method)(Args...) const, ActualArgs... args) const
    {
        return detail::forward<Ret>(this, (sourceModel()->*method)(detail::reverse<Args>(this, args)...));
    }
};

} // namespace glaxnimate::gui::item_models
