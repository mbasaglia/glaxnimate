/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QTreeView>
#include <QMouseEvent>

namespace glaxnimate::gui {

/**
 * \brief QTreeView but slightly different mouse actions
 */
class CustomTreeView : public QTreeView
{
public:
    using QTreeView::QTreeView;

protected:
    void mousePressEvent(QMouseEvent * event) override
    {
        if ( event->button() != Qt::RightButton )
            QTreeView::mousePressEvent(event);
    }


    void mouseReleaseEvent(QMouseEvent * event) override
    {
        if ( event->button() == Qt::RightButton )
            emit customContextMenuRequested(event->pos());
        else
            QTreeView::mouseReleaseEvent(event);
    }
};

} // namespace glaxnimate::gui
