/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>

#include <QLayout>
#include <QStyle>
#include <QBoxLayout>

namespace glaxnimate::gui {

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(int items_per_row = 3, int min_w = 32, int max_w = 80, QWidget *parent = nullptr);
    ~FlowLayout();

    void addItem(QLayoutItem *item) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    void set_fixed_item_size(const QSize& size);
    void set_orientation(Qt::Orientation orientation);

private:
    QSize do_layout(const QRect &rect, bool test_only) const;
    bool valid_index(int index) const;

    std::vector<QLayoutItem *> items;
    int min_w;
    int max_w;
    int items_per_row;
    QSize fixed_size;
    Qt::Orientation orient = Qt::Horizontal;
    mutable QSize contents;
};

} // namespace glaxnimate::gui
