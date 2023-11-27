/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "log_model.hpp"
#include "app/utils/qstring_literal.hpp"

namespace {
    enum Columns
    {
        Time,
        Source,
        SourceDetail,
        Message,

        Count
    };
} // namespace

app::log::LogModel::LogModel()
{
    connect(&Logger::instance(), &Logger::logged, this, &LogModel::on_line);
}

int app::log::LogModel::rowCount(const QModelIndex &) const
{
    return lines.size();
}

int app::log::LogModel::columnCount(const QModelIndex &) const
{
    return Count;
}

QVariant app::log::LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal )
    {
        if ( role == Qt::DisplayRole )
        {
            switch ( section )
            {
                case Time:
                    return tr("Time");
                case Source:
                    return tr("Source");
                case SourceDetail:
                    return tr("Details");
                case Message:
                    return tr("Message");
            }
        }
    }
    else
    {
        if ( role == Qt::DecorationRole )
        {
            switch ( lines[section].severity )
            {
                case Info: return QIcon::fromTheme("emblem-information"_qs);
                case Warning: return QIcon::fromTheme("emblem-warning"_qs);
                case Error: return QIcon::fromTheme("emblem-error"_qs);
            }
        }
        else if ( role == Qt::ToolTipRole )
        {
            return Logger::severity_name(lines[section].severity);
        }
    }

    return {};
}

QVariant app::log::LogModel::data(const QModelIndex & index, int role) const
{
    if ( !index.isValid() )
        return {};

    const LogLine& line = lines[index.row()];
    if ( role == Qt::DisplayRole )
    {
        switch ( index.column() )
        {
            case Time:
                return line.time.toString(Qt::ISODate);
            case Source:
                return line.source;
            case SourceDetail:
                return line.source_detail;
            case Message:
                return line.message;
        }
    }
    else if ( role == Qt::ToolTipRole )
    {
        switch ( index.column() )
        {
            case Time:
                return line.time.toString();
            case SourceDetail:
                return line.source_detail;
        }
    }

    return {};
}

void app::log::LogModel::on_line(const LogLine& line)
{
    beginInsertRows(QModelIndex(), lines.size(), lines.size());
    lines.push_back(line);
    endInsertRows();
}

void app::log::LogModel::populate(const std::vector<LogLine>& lines)
{
    beginResetModel();
    this->lines = lines;
    endResetModel();
}
