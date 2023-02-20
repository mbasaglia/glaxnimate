/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "keyboard_shortcuts_model.hpp"
#include "app/widgets/clearable_keysequence_edit.hpp"

app::settings::KeyboardShortcutsModel::KeyboardShortcutsModel(app::settings::ShortcutSettings* settings, QObject* parent)
    : QAbstractItemModel(parent), settings(settings)
{}

int app::settings::KeyboardShortcutsModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int app::settings::KeyboardShortcutsModel::rowCount(const QModelIndex& parent) const
{
    if ( !parent.isValid() )
        return settings->get_groups().size();

    if ( !parent.parent().isValid() && parent.row() < settings->get_groups().size() )
    {
        auto& grp = settings->get_groups()[parent.row()];
        return grp.actions.size();
    }

    return 0;
}

QVariant app::settings::KeyboardShortcutsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return section == 0 ? tr("Name") : tr("Shortcut");
    return {};
}

Qt::ItemFlags app::settings::KeyboardShortcutsModel::flags(const QModelIndex& index) const
{
    auto flags = QAbstractItemModel::flags(index);

    if ( index.isValid() && index.parent().isValid() && index.column() == 1 )
        flags |= Qt::ItemIsEditable;

    return flags;
}

QVariant app::settings::KeyboardShortcutsModel::data(const QModelIndex& index, int role) const
{
    if ( !index.isValid() )
        return {};

    if ( !index.parent().isValid() )
    {
        if ( role != Qt::DisplayRole || index.column() != 0 || index.row() >= settings->get_groups().size() )
            return {};

        auto& grp = settings->get_groups()[index.row()];
        return grp.label;
    }

    int grp_index = index.internalId();
    if ( grp_index >= settings->get_groups().size() )
        return {};

    auto& grp = settings->get_groups()[grp_index];
    if ( index.row() >= int(grp.actions.size()) )
        return {};

    auto act = grp.actions[index.row()];
    if ( index.column() == 0 )
    {
        switch ( role )
        {
            case Qt::DisplayRole:
                return act->label;
            case Qt::DecorationRole:
                return act->icon;
            default:
                return {};
        }
    }
    else
    {
        switch ( role )
        {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return act->shortcut;
            case DefaultKeyRole:
                return act->default_shortcut;
            default:
                return {};
        }
    }
}

bool app::settings::KeyboardShortcutsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ( !index.isValid() )
        return false;

    if ( !index.parent().isValid() )
        return false;

    int grp_index = index.internalId();
    if ( grp_index >= settings->get_groups().size() )
        return false;

    if ( index.column() != 1 || role != Qt::EditRole )
        return false;

    auto& grp = settings->get_groups()[grp_index];
    if ( index.row() >= int(grp.actions.size()) )
        return false;

    auto act = grp.actions[index.row()];

    QKeySequence ks;

    if ( value.canConvert<QKeySequence>() )
        ks = value.value<QKeySequence>();
    else if ( value.canConvert<QString>() )
        ks = QKeySequence(value.toString(), QKeySequence::PortableText);
    else
        return false;

    act->overwritten = ks != act->default_shortcut;
    act->shortcut = ks;
    if ( act->action )
        act->action->setShortcut(ks);
    emit dataChanged(index, index, {role});
    return true;
}


QModelIndex app::settings::KeyboardShortcutsModel::index(int row, int column, const QModelIndex& parent) const
{
    if ( !parent.isValid() )
    {
        if ( row >= settings->get_groups().size() )
            return {};
        return createIndex(row, column, 1000 + row);
    }

    return createIndex(row, column, parent.internalId()-1000);
}

QModelIndex app::settings::KeyboardShortcutsModel::parent(const QModelIndex& child) const
{
    if ( !child.isValid() )
        return {};

    int id = child.internalId();
    if ( id >= 1000 )
        return {};

    return createIndex(id, 0, id+1000);
}

app::settings::ShortcutAction * app::settings::KeyboardShortcutsModel::action(const QModelIndex& index) const
{
    if ( !index.isValid() )
        return nullptr;

    if ( !index.parent().isValid() )
        return nullptr;

    int grp_index = index.internalId();
    if ( grp_index >= settings->get_groups().size() )
        return nullptr;

    auto& grp = settings->get_groups()[grp_index];
    if ( index.row() >= int(grp.actions.size()) )
        return nullptr;

    return grp.actions[index.row()];
}

void app::settings::KeyboardShortcutsModel::begin_change_data()
{
    emit beginResetModel();
}

void app::settings::KeyboardShortcutsModel::end_change_data()
{
    emit endResetModel();
}

bool app::settings::KeyboardShortcutsFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if ( !source_parent.isValid() )
        return true;

#if QT_VERSION_MAJOR >= 6
    auto re = filterRegularExpression();
#else
    auto re = filterRegExp();
#endif

    QModelIndex i0 = sourceModel()->index(source_row, 0, source_parent);
    QModelIndex i1 = sourceModel()->index(source_row, 1, source_parent);

    return sourceModel()->data(i0).toString().contains(re) || sourceModel()->data(i1).toString().contains(re);
}

QWidget * app::settings::KeyboardShortcutsDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if ( index.data(Qt::EditRole).canConvert<QKeySequence>() )
        return new ClearableKeysequenceEdit(parent);
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void app::settings::KeyboardShortcutsDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QVariant edit_data = index.data(Qt::EditRole);
    if ( edit_data.canConvert<QKeySequence>() )
    {
        ClearableKeysequenceEdit* widget = static_cast<ClearableKeysequenceEdit*>(editor);
        widget->set_key_sequence(edit_data.value<QKeySequence>());
        QVariant default_data = index.data(KeyboardShortcutsModel::DefaultKeyRole);
        if ( default_data.canConvert<QKeySequence>() )
            widget->set_default_key_sequence(default_data.value<QKeySequence>());
    }
    return QStyledItemDelegate::setEditorData(editor, index);
}

void app::settings::KeyboardShortcutsDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if ( index.data(Qt::EditRole).canConvert<QKeySequence>() )
        model->setData(index, static_cast<ClearableKeysequenceEdit*>(editor)->key_sequence());
    return QStyledItemDelegate::setModelData(editor, model, index);
}


