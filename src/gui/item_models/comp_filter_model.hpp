#pragma once

#include "proxy_base.hpp"
#include "document_node_model.hpp"

namespace item_models {

class CompFilterModel : public ProxyBase<CompFilterModel>
{
private:
    friend Ctor;
    void source_changed(QAbstractItemModel *)
    {
        root_id = 0;
        root = QModelIndex();
    }

public:
    using Ctor::Ctor;

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
    void on_clear()
    {
        if ( root_id && !root.isValid() )
            set_root({});
    }


private:
    quintptr root_id = 0;
    QPersistentModelIndex root;
};

} // namespace item_models
