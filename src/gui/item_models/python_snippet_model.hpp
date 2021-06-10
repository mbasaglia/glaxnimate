#pragma once

#include <QAbstractListModel>
#include "plugin/snippet.hpp"

namespace item_models {

class PythonSnippetModel : public QAbstractListModel
{
public:
    /**
     * \brief Reloads all snippets
     */
    void reload()
    {
        beginResetModel();
        snippets.clear();
        QDir dir(plugin::Snippet::snippet_path());
        for ( const auto& sub : dir.entryInfoList({"*.py"}, QDir::Files|QDir::Readable|QDir::Writable, QDir::Name) )
        {
            snippets.emplace_back(sub.baseName());
        }
        endResetModel();
    }

    QModelIndex append()
    {
        beginInsertRows({}, snippets.size(), snippets.size());
        QString name_template = tr("New Snippet %1");
        QString name = name_template.arg("").trimmed();
        for ( int i = 1; name_used(name); i++ )
            name = name_template.arg(i);
        snippets.emplace_back(name);
        endInsertRows();
        return createIndex(snippets.size() - 1, 0);
    }

    int rowCount(const QModelIndex &) const override
    {
        return snippets.size();
    }

    Qt::ItemFlags flags(const QModelIndex &) const override
    {
        return Qt::ItemIsEnabled|Qt::ItemIsEditable|Qt::ItemIsSelectable|Qt::ItemNeverHasChildren;
    }

    QVariant data(const QModelIndex & index, int role) const override
    {
        if ( index.row() >= 0 && index.row() < int(snippets.size()) )
        {
            switch ( role )
            {
                case Qt::DisplayRole:
                case Qt::EditRole:
                    return snippets[index.row()].name();
                case Qt::DecorationRole:
                    return QIcon::fromTheme("text-x-python");
                case Qt::ToolTipRole:
                    return snippets[index.row()].filename();
            }
        }

        return {};
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role) override
    {
        if ( index.row() >= 0 && index.row() < int(snippets.size()) && role == Qt::EditRole )
        {
            QString name = snippets[index.row()].set_name(value.toString());
            emit dataChanged(index, index, {});
            return true;
        }

        return false;
    }

    plugin::Snippet snippet(const QModelIndex& index)
    {
        if ( index.row() >= 0 && index.row() < int(snippets.size()) )
            return snippets[index.row()];
        return plugin::Snippet{};
    }

    bool removeRows(int row, int count, const QModelIndex & index) override
    {
        if ( row > 0 && count > 0 && row + count <= int(snippets.size()) )
        {
            beginRemoveRows(index, row, row + count);
            snippets.erase(snippets.begin() + row, snippets.begin() + row + count);
            endRemoveRows();
            return true;
        }

        return false;
    }

private:
    bool name_used(const QString& name) const
    {
        for ( const auto & snip : snippets )
            if ( snip.name() == name )
                return true;
        return false;
    }

    std::vector<plugin::Snippet> snippets;
};

} // namespace item_models
