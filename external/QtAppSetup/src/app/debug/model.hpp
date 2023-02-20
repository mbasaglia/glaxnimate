/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

    struct DebugSlot
    {
        QString prefix;
        QString signal;

        template<class T>
        decltype(std::declval<QDebug&>() << std::declval<T>()) print(QDebug& stream, T&& t, bool)
        {
            return stream << std::forward<T>(t);
        }

        template<class T>
        void print(QDebug&, T&&, ...){}
        void print_all(QDebug&) {}

        template<class T, class... Args>
        void print_all(QDebug& stream, T&& t, Args&&... args)
        {
            print(stream, std::forward<T>(t), true);
            print_all(stream, std::forward<Args>(args)...);
        }

        template<class... Args>
        void operator()(Args&&... args)
        {
            auto stream = qDebug();
            if ( !prefix.isEmpty() )
                stream << prefix;
            stream << signal;
            print_all(stream, std::forward<Args>(args)...);
        }
    };

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

inline void connect_debug(QAbstractItemModel* model, const QString& prefix)
{
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeInserted,  model, detail::DebugSlot{prefix, "columnsAboutToBeInserted"});
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeMoved,     model, detail::DebugSlot{prefix, "columnsAboutToBeMoved"});
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,   model, detail::DebugSlot{prefix, "columnsAboutToBeRemoved"});
    QObject::connect(model, &QAbstractItemModel::columnsInserted,           model, detail::DebugSlot{prefix, "columnsInserted"});
    QObject::connect(model, &QAbstractItemModel::columnsMoved,              model, detail::DebugSlot{prefix, "columnsMoved"});
    QObject::connect(model, &QAbstractItemModel::columnsRemoved,            model, detail::DebugSlot{prefix, "columnsRemoved"});
    QObject::connect(model, &QAbstractItemModel::dataChanged,               model, detail::DebugSlot{prefix, "dataChanged"});
    QObject::connect(model, &QAbstractItemModel::headerDataChanged,         model, detail::DebugSlot{prefix, "headerDataChanged"});
    QObject::connect(model, &QAbstractItemModel::layoutAboutToBeChanged,    model, detail::DebugSlot{prefix, "layoutAboutToBeChanged"});
    QObject::connect(model, &QAbstractItemModel::layoutChanged,             model, detail::DebugSlot{prefix, "layoutChanged"});
    QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset,       model, detail::DebugSlot{prefix, "modelAboutToBeReset"});
    QObject::connect(model, &QAbstractItemModel::modelReset,                model, detail::DebugSlot{prefix, "modelReset"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeInserted,     model, detail::DebugSlot{prefix, "rowsAboutToBeInserted"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeMoved,        model, detail::DebugSlot{prefix, "rowsAboutToBeMoved"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,      model, detail::DebugSlot{prefix, "rowsAboutToBeRemoved"});
    QObject::connect(model, &QAbstractItemModel::rowsInserted,              model, detail::DebugSlot{prefix, "rowsInserted"});
    QObject::connect(model, &QAbstractItemModel::rowsMoved,                 model, detail::DebugSlot{prefix, "rowsMoved"});
    QObject::connect(model, &QAbstractItemModel::rowsRemoved,               model, detail::DebugSlot{prefix, "rowsRemoved"});
}

} // namespace app::debug
