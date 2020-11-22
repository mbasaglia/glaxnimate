#pragma once

#include <QSortFilterProxyModel>
#include "model/composition.hpp"

namespace item_models {

class CompFilterModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void set_composition(model::Composition* composition)
    {
        comp = composition;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if ( source_parent.isValid() )
            return true;

        return sourceModel()->index(source_row, 0, source_parent).internalPointer() == comp;
    }

private:
    model::Composition* comp = nullptr;
};

} // namespace item_models
