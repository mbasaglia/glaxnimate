/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "flow_layout.hpp"
#include <QtMath>

using namespace glaxnimate::gui;


FlowLayout::FlowLayout(int items_per_row, int min_w, int max_w, QWidget* parent)
    : QLayout(parent), min_w(min_w), max_w(max_w), items_per_row(items_per_row)
{
}

FlowLayout::~FlowLayout()
{
    for ( auto it : items )
        delete it;
}

void FlowLayout::addItem(QLayoutItem* item)
{
    items.push_back(item);
}

int FlowLayout::count() const
{
    return items.size();
}

bool FlowLayout::valid_index(int index) const
{
    return index >= 0 && index < int(items.size());
}

QLayoutItem * FlowLayout::itemAt(int index) const
{
    if ( !valid_index(index) )
        return nullptr;
    return items[index];
}

QLayoutItem * FlowLayout::takeAt(int index)
{
    if ( valid_index(index) )
    {
        auto p = items[index];
        items.erase(items.begin() + index);
        return p;
    }

    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
#ifdef Q_OS_ANDROID
    return orient;
#else
    return QLayout::expandingDirections();
#endif
}


bool FlowLayout::hasHeightForWidth() const
{
    return orient == Qt::Horizontal;
}

int FlowLayout::heightForWidth(int width) const
{
    return do_layout(QRect(0, 0, width, 0), true).height();
}

QSize FlowLayout::do_layout(const QRect& rect, bool test_only) const
{
    if ( items.size() == 0 )
        return QSize(0, 0);

    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effective_rect = rect.adjusted(+left, +top, -right, -bottom);

    int vertical_spacing = spacing();
    int horizontal_spacing = spacing();

    int ipr = items_per_row;
    int iw = (effective_rect.width() - horizontal_spacing * (ipr - 1)) / ipr;
    int nrows;
    int ih;

    if ( fixed_size.isValid() )
    {
        iw = fixed_size.width();
        ih = fixed_size.height();
        if ( orient == Qt::Horizontal )
        {
            ipr =  qMax(1, (effective_rect.width() + horizontal_spacing) / (iw + horizontal_spacing));
            nrows = (items.size() + ipr - 1) / ipr;
        }
        else
        {
            nrows = qMax(1, (effective_rect.height() + vertical_spacing) / (ih + vertical_spacing));;
            ipr =  (items.size() + nrows - 1) / nrows;
        }
    }
    else
    {
        if ( iw < min_w )
        {
            iw = min_w;
            ipr = (effective_rect.width() + horizontal_spacing) / (min_w + horizontal_spacing);
        }
        else if ( iw > max_w )
        {
            ipr = qMin<int>(
                items.size(),
                qCeil(qreal(effective_rect.width() + horizontal_spacing) / (max_w + horizontal_spacing))
            );
            iw = qMin(max_w, (effective_rect.width() - horizontal_spacing * (ipr - 1)) / ipr);
        }

        if ( ipr == 0 )
        {
            ipr = items_per_row;
            iw = (effective_rect.width() - horizontal_spacing * (ipr - 1)) / ipr;
        }

        nrows = (items.size() + ipr - 1) / ipr;
        ih = (effective_rect.height() - vertical_spacing * (nrows - 1)) / nrows;
        if ( !test_only && ih < iw )
        {
            iw = qMax(ih, min_w);
            ipr = (effective_rect.width() + horizontal_spacing) / (iw + horizontal_spacing);
        }
    }

    QSize new_contents(
        ipr * iw + (ipr-1) * horizontal_spacing,
        nrows * ih + (nrows-1) * vertical_spacing
    );
    if ( test_only )
        return new_contents;

    contents = new_contents;

    QSize item_size(iw, ih);

    int x = 0;
    int y = 0;
    for ( int i = 0; i < int(items.size()); i++ )
    {
        QLayoutItem *item = items[i];

        if ( !test_only )
            item->setGeometry(QRect(effective_rect.topLeft() + QPoint(x, y), item_size));

        if ( orient == Qt::Horizontal )
        {
            if ( i % ipr == ipr-1 )
            {
                x = 0;
                y += item_size.height() + vertical_spacing;
            }
            else
            {
                x += item_size.width() + horizontal_spacing;
            }
        }
        else
        {
            if ( i % nrows == nrows-1 )
            {
                y = 0;
                x += item_size.width() + vertical_spacing;
            }
            else
            {
                y += item_size.height() + horizontal_spacing;
            }
        }
    }

    return contents;
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    do_layout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    if ( contents.isValid() )
    {
        const QMargins margins = contentsMargins();
        return contents + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for ( const QLayoutItem *item : items )
        size = size.expandedTo(item->minimumSize());

    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());

    return size;
}

void FlowLayout::set_fixed_item_size(const QSize& size)
{
    fixed_size = size;
    invalidate();
}

void FlowLayout::set_orientation(Qt::Orientation orientation)
{
    orient = orientation;
    invalidate();
}
