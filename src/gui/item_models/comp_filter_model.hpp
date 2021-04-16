#pragma once

#include <QAbstractProxyModel>
#include "document_node_model.hpp"

namespace item_models {

class CompFilterModel : public QAbstractProxyModel
{
public:
    using QAbstractProxyModel::QAbstractProxyModel;

    void setSourceModel(QAbstractItemModel *new_model) override
    {
        beginResetModel();

        if ( sourceModel() )
            disconnect(sourceModel(), nullptr, this, nullptr);

        root_id = 0;
        root = QModelIndex();

        QAbstractProxyModel::setSourceModel(new_model);

        if ( sourceModel() )
        {
            connect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &CompFilterModel::on_clear);
            connect(sourceModel(), &QAbstractItemModel::columnsRemoved, this, &CompFilterModel::on_clear);

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

        endResetModel();
    }

    void set_composition(model::Composition* comp)
    {
        if ( !sourceModel() )
            return;

        set_root(friendly_model()->node_index(comp));
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

        return friendly_model()->createIndex(row, proxyIndex.column(), proxyIndex.internalId());
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
    // Qt cheats by befriending the proxy models... so we need to cheat too
    DocumentNodeModel* friendly_model() const
    {
        return static_cast<DocumentNodeModel*>(sourceModel());
    }

    void on_clear()
    {
        if ( root_id && !root.isValid() )
            set_root({});
    }

    template<class... Args>
    class SignalForwarder
    {
    public:
        using Functor = void (QAbstractItemModel::*)(Args...);
        CompFilterModel* proxy;
        Functor signal;

        SignalForwarder(CompFilterModel* proxy, Functor signal)
            : proxy(proxy), signal(signal)
        {}

        template<class T>
        inline std::enable_if_t<!std::is_same_v<std::decay_t<T>, QModelIndex>, T>  forward(T v) const
        {
            return v;
        }

        template<class T>
        inline std::enable_if_t<std::is_same_v<std::decay_t<T>, QModelIndex>, QModelIndex>  forward(T v) const
        {
            return proxy->mapFromSource(v);
        }

        inline void operator()(Args... args) const
        {
            (proxy->*signal)(forward<Args>(args)...);
        }
    };

    template<class... Args>
    void forward_signal(void (QAbstractItemModel::*signal)(Args...))
    {
        connect(sourceModel(), signal, this, SignalForwarder(this, signal));

    }


private:
    quintptr root_id = 0;
    QPersistentModelIndex root;
};

} // namespace item_models
