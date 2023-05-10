/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QSortFilterProxyModel>

#include "document_node_model.hpp"

#include "model/assets/assets.hpp"

namespace glaxnimate::gui::item_models {

class NodeTypeProxyModel: public QSortFilterProxyModel
{
public:
    NodeTypeProxyModel(DocumentNodeModel* model)
    {
        setSourceModel(model);
    }

    template<class T>
    void allow()
    {
        allowed.push_back(&T::staticMetaObject);
        invalidateFilter();
    }


    DocumentNodeModel* source_model() const
    {
        return static_cast<DocumentNodeModel*>(sourceModel());
    }

    Qt::ItemFlags flags(const QModelIndex & index) const override
    {
        auto src = mapToSource(index);
        auto flags = source_model()->flags(src) & ~(Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
        auto node = source_model()->node(src);
        bool selectable = false;

        if ( node )
        {
            if ( allowed.empty() )
            {
                selectable = true;
            }
            else
            {
                auto meta = node->metaObject();
                for ( auto mo : allowed )
                {
                    if ( meta->inherits(mo) )
                    {
                        selectable = true;
                        break;
                    }
                }
            }
        }

        if ( selectable )
            flags |= Qt::ItemIsSelectable;
        else
            flags &= ~Qt::ItemIsSelectable;

        return flags;
    }

    model::DocumentNode* node(const QModelIndex& index) const
    {
        return source_model()->node(mapToSource(index));
    }

protected:

    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override
    {
        auto node = source_model()->node(source_model()->index(source_row, 0, source_parent));
        if ( !node )
            return false;

        auto meta = node->metaObject();

        for ( auto mo : implicit )
            if ( meta->inherits(mo) )
                return true;

        for ( auto mo : allowed )
            if ( meta->inherits(mo) )
                return true;

        return false;
    }



private:
    std::vector<const QMetaObject*> implicit = {
        &model::Group::staticMetaObject,
        &model::Composition::staticMetaObject,
        &model::Assets::staticMetaObject,
        &model::CompositionList::staticMetaObject,
    };
    std::vector<const QMetaObject*> allowed;
};

} // namespace glaxnimate::gui::item_models
