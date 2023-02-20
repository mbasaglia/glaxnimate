/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QStyledItemDelegate>

namespace glaxnimate::gui::style {

class FixedHeightDelegate : public QStyledItemDelegate
{
public:
    explicit FixedHeightDelegate(qreal height = -1) : height (height) {}

    void set_height(qreal height)
    {
        this->height = height;
    }

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override
    {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(height);
        return size;
    }

private:
    qreal height;
};

} // namespace glaxnimate::gui::style
