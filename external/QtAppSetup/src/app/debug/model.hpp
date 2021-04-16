#pragma once
#include <QDebug>
#include <QAbstractItemModel>

namespace app::debug {

namespace detail {

    inline void print_model_column(const QAbstractItemModel* model, int i, bool flags, const std::vector<int>& roles, const QModelIndex& index, int indent)
    {
        auto logger = qDebug();
        logger.noquote();
        auto colindex = model->index(index.row(), i, index.parent());
        for ( int role : roles )
        {
            logger << QString(4*indent, ' ') << "  *" << model->data(colindex, role);
            if ( flags )
                logger << model->flags(colindex);
        }
    }

} // namespace detail

inline void print_model_row(const QAbstractItemModel* model, const QModelIndex& index, const std::vector<int>& columns = {}, bool flags = false, const std::vector<int>& roles = {Qt::DisplayRole}, int indent = 0)
{
    int rows = model->rowCount(index);
    int cols = model->columnCount(index);

    qDebug().noquote() << QString(4*indent, ' ') << index << "rows" << rows << "cols" << cols << "ptr" << index.internalId();

    if ( columns.empty() )
    {
        for ( int i = 0; i < cols; i++ )
            detail::print_model_column(model, i, flags, roles, index, indent);
    }
    else
    {
        for ( int i : columns )
            detail::print_model_column(model, i, flags, roles, index, indent);
    }
}

inline void print_model(const QAbstractItemModel* model, const std::vector<int>& columns = {}, bool flags = false, const std::vector<int>& roles = {Qt::DisplayRole}, const QModelIndex& index = {}, int indent = 0)
{
    print_model_row(model, index, columns, flags, roles, indent);
    for ( int i = 0; i < model->rowCount(index); i++ )
    {
        QModelIndex ci = model->index(i, 0, index);
        if ( !ci.isValid() )
            qDebug().noquote() << QString(4*(indent+1), ' ') << "invalid";
        else
            print_model(model, columns, flags, roles, ci, indent+1);
    }
}

} // namespace app::debug
