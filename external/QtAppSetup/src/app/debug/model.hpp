#pragma once
#include <QDebug>
#include <QAbstractItemModel>

namespace app::debug {

inline void print_model(QAbstractItemModel* model, const QModelIndex& index = {}, const std::vector<int>& roles = {Qt::DisplayRole}, int indent = 0)
{
    int rows = model->rowCount(index);
    {
        int cols = model->columnCount(index);
        auto logger = qDebug();
        logger << QString(2*indent, ' ') << index << "rows" << rows << "cols" << cols << "data";
        for ( int role : roles )
            logger << model->data(index, role);
        logger << "flags";
        for ( int i = 0; i < cols; i++ )
            logger << model->flags(model->index(index.row(), i, index.parent()));
    }
    for ( int i = 0; i < rows; i++ )
    {
        QModelIndex ci = model->index(i, 0, index);
        if ( !ci.isValid() )
            qDebug() << QString(2*(indent+1), ' ') << "invalid";
        else
            print_model(model, ci, roles, indent+1);
    }
}

} // namespace app::debug
