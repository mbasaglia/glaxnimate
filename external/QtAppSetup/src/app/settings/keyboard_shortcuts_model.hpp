/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

#include "keyboard_shortcuts.hpp"

namespace app::settings {

class KeyboardShortcutsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum {
        DefaultKeyRole = Qt::UserRole
    };

    KeyboardShortcutsModel(ShortcutSettings* settings, QObject* parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int columnCount(const QModelIndex & parent) const override;
    int rowCount(const QModelIndex & parent) const override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role) override;

    QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    QModelIndex parent(const QModelIndex & child) const override;

    ShortcutAction* action(const QModelIndex & index) const;

public slots:
    void begin_change_data();
    void end_change_data();

private:
    ShortcutSettings* settings;
};

class KeyboardShortcutsFilterModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
};

class KeyboardShortcutsDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
    void setEditorData(QWidget * editor, const QModelIndex & index) const override;
    void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override;
};

} // namespace app::settings
