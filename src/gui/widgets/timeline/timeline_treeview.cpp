#include "timeline_treeview.hpp"

#include <QMouseEvent>
#include <QPainter>

#include "item_models/property_model_full.hpp"
#include "item_models/comp_filter_model.hpp"

class glaxnimate::gui::TimelineTreeview::Private
{
public:
    QModelIndex source_index(const QModelIndex& index)
    {
        return proxy_model()->mapToSource(index);
    }

    item_models::CompFilterModel* proxy_model()
    {
        return static_cast<item_models::CompFilterModel*>(parent->model());
    }

    item_models::PropertyModelFull* source_model()
    {
        return static_cast<item_models::PropertyModelFull*>(proxy_model()->sourceModel());
    }

    model::Layer* layer(const QModelIndex& source_index)
    {
        return qobject_cast<model::Layer*>(source_model()->node(source_index));
    }

    void on_drag()
    {
        auto index = parent->indexAt(drag_to_local);
        auto source_index = this->source_index(index);

        auto layer = this->layer(source_index);

        if ( layer && drag_from_layer->is_valid_parent(layer) )
        {
            drag_to_layer = layer;
            drag_to_index = index;
        }
        else
        {
            drag_to_layer = nullptr;
            drag_to_index = {};
        }

        parent->viewport()->update();
    }

    QPoint relpoint()
    {
        return parent->visualRect(parent->model()->index(0, 0)).topLeft();
    }

    TimelineTreeview* parent;
    QPoint drag_from;
    QPoint drag_to;
    QPoint drag_to_local;
    QModelIndex drag_from_index;
    QModelIndex drag_to_index;
    model::Layer* drag_from_layer = nullptr;
    model::Layer* drag_to_layer = nullptr;
    bool dragging = false;
};

glaxnimate::gui::TimelineTreeview::TimelineTreeview(QWidget* parent)
    : QTreeView(parent), d(std::make_unique<Private>())
{
    d->parent = this;
}

glaxnimate::gui::TimelineTreeview::~TimelineTreeview() = default;

void glaxnimate::gui::TimelineTreeview::mousePressEvent(QMouseEvent* event)
{
    if ( event->button() == Qt::LeftButton )
    {
        auto index = indexAt(event->pos());
        auto source_index = d->source_index(index);

        if ( source_index.column() == item_models::PropertyModelFull::ColumnValue )
        {
            auto layer = d->layer(source_index);
            if ( layer && !layer->docnode_locked_recursive()  )
            {
                d->drag_from_layer = layer;
                d->drag_from = d->drag_to = event->pos() - d->relpoint();
                d->drag_to_local = event->pos();
                d->drag_from_index = index;
                d->drag_to_layer = nullptr;
                d->drag_to_layer = nullptr;
                d->drag_to_index = {};
                d->dragging = true;

                event->accept();
                viewport()->update();
                return;
            }
        }
    }

    QTreeView::mouseMoveEvent(event);
}

void glaxnimate::gui::TimelineTreeview::mouseMoveEvent(QMouseEvent* event)
{
    if ( d->dragging )
    {
        d->drag_to = event->pos() - d->relpoint();
        d->drag_to_local = event->pos();
        d->on_drag();
    }
    else
    {
        QTreeView::mouseMoveEvent(event);
    }
}

void glaxnimate::gui::TimelineTreeview::mouseReleaseEvent(QMouseEvent* event)
{
    if ( d->dragging && event->button() == Qt::LeftButton )
    {
        d->dragging = false;

        // if moved less than 3 pixels, treat as a click
        if ( math::length_squared<QPointF>(d->drag_to - d->drag_from) < 9 )
        {
            QTreeView::mousePressEvent(event);
            QTreeView::mouseReleaseEvent(event);
        }
        else
        {
            d->drag_from_layer->parent.set_undoable(QVariant::fromValue(d->drag_to_layer));
        }

        d->drag_from_layer = d->drag_to_layer = nullptr;
        d->drag_from_index = d->drag_to_index = {};
        viewport()->update();
    }
    else
    {
        QTreeView::mouseReleaseEvent(event);
    }
}

void glaxnimate::gui::TimelineTreeview::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);

    if ( d->dragging )
    {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        if ( d->drag_to_index.isValid() )
        {
            painter.setPen(QPen(palette().text(), 2));

            auto rect = visualRect(d->drag_to_index);
            rect.setX(0);
            rect.setWidth(width());
            painter.drawRect(rect);
        }

        painter.setPen(QPen(palette().highlight(), 2));
        QPoint off = d->relpoint();
        painter.drawLine(
            d->drag_from + off,
            d->drag_to + off
        );
    }
}

void glaxnimate::gui::TimelineTreeview::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);

    if ( d->dragging )
    {
        d->drag_to = d->drag_to_local - d->relpoint();
        d->on_drag();
    }
}
