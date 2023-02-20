/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractItemDelegate>
#include <QFontDatabase>

namespace glaxnimate::gui::font {


class FontDelegate : public QAbstractItemDelegate
{
public:
    explicit FontDelegate(QObject *parent = nullptr);

    // painting
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

//     const QIcon truetype;
//     const QIcon bitmap;
    QFontDatabase::WritingSystem writingSystem;
};

} // namespace glaxnimate::gui::font
