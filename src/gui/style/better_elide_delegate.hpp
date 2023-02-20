/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QStyledItemDelegate>
#include <QApplication>

namespace glaxnimate::gui::style {
/*
 * Dunno why but by default the table view messes up text elision,
 * this fixes the issue and also allows for different text elision modes
 * in the same view
 */
class BetterElideDelegate : public QStyledItemDelegate
{
public:
    BetterElideDelegate(Qt::TextElideMode mode, QObject* parent =  nullptr)
        : QStyledItemDelegate(parent), mode(mode) {}

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.textElideMode = Qt::ElideNone;
        const QWidget* widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        const int text_margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;
        opt.text = opt.fontMetrics.elidedText(opt.text, mode, opt.rect.width() - text_margin * 2, 0);
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }

private:
    Qt::TextElideMode mode;
};

} // namespace glaxnimate::gui::style
