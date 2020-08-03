#include "transform_graphics_item.hpp"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include "model/document.hpp"
#include "command/property_commands.hpp"

class model::graphics::TransformGraphicsItem::Private
{
public:
    enum Index
    {
        TL,
        TR,
        BR,
        BL,
        Top,
        Bottom,
        Left,
        Right,
        Anchor,
        Rot,

        Count
    };

    struct Handle
    {
        MoveHandle* handle;
        QPointF (Private::* get_p)()const;
        void (TransformGraphicsItem::* signal)(const QPointF&);

    };

    Transform* transform;
    DocumentNode* target;
    std::array<Handle, Count> handles;
    QRectF cache;
    QTransform tranform_matrix;
    QTransform tranform_matrix_inv;

    QPointF get_tl() const { return cache.topLeft(); }
    QPointF get_tr() const { return cache.topRight(); }
    QPointF get_br() const { return cache.bottomRight(); }
    QPointF get_bl() const { return cache.bottomLeft(); }
    QPointF get_t() const { return {cache.center().x(), cache.top()}; }
    QPointF get_b() const { return {cache.center().x(), cache.bottom()}; }
    QPointF get_l() const { return {cache.left(), cache.center().y()}; }
    QPointF get_r() const { return {cache.right(), cache.center().y()}; }
    QPointF get_a() const
    {
        return transform->anchor_point.get();
    }
    QPointF get_rot() const
    {
        return {
            cache.center().x(),
            cache.top() - 32 / transform->scale.get().y()
        };
    }

    void set_pos(const Handle& h) const
    {
        h.handle->setPos((this->*h.get_p)());
    }

    Private(TransformGraphicsItem* parent, model::Transform* transform, model::DocumentNode* target)
    : transform(transform),
        target(target),
        handles{
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalDown, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_tl,
                &TransformGraphicsItem::drag_tl
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalUp, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_tr,
                &TransformGraphicsItem::drag_tr
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalDown, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_br,
                &TransformGraphicsItem::drag_br
            },
            Handle{
                new MoveHandle(parent, MoveHandle::DiagonalUp, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_bl,
                &TransformGraphicsItem::drag_bl
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Vertical, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_t,
                &TransformGraphicsItem::drag_t
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Vertical, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_b,
                &TransformGraphicsItem::drag_b
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Horizontal, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_l,
                &TransformGraphicsItem::drag_l
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Horizontal, MoveHandle::Diamond, 8, true),
                &TransformGraphicsItem::Private::get_r,
                &TransformGraphicsItem::drag_r
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Any, MoveHandle::Square, 6, true),
                &TransformGraphicsItem::Private::get_a,
                &TransformGraphicsItem::drag_a
            },
            Handle{
                new MoveHandle(parent, MoveHandle::Rotate, MoveHandle::Circle, 6, true),
                &TransformGraphicsItem::Private::get_rot,
                &TransformGraphicsItem::drag_rot
            },
        }
    {
    }

    qreal find_scale(qreal target, qreal original, qreal size)
    {
        qreal delta = target - original;
        qreal w1 = size + 2*delta;
        return w1 / size;
    }

};

model::graphics::TransformGraphicsItem::TransformGraphicsItem(
    model::Transform* transform, model::DocumentNode* target, QGraphicsItem* parent
)
    : QGraphicsObject(parent), d(std::make_unique<Private>(this, transform, target))
{
    connect(target, &DocumentNode::bounding_rect_changed, this, &TransformGraphicsItem::update_handles);
    connect(transform, &Object::property_changed, this, &TransformGraphicsItem::update_transform);
    update_handles();
    update_transform();
    for ( const auto& h : d->handles )
        connect(h.handle, &MoveHandle::dragged, this, h.signal);
}

model::graphics::TransformGraphicsItem::~TransformGraphicsItem() = default;

void model::graphics::TransformGraphicsItem::update_handles()
{
    prepareGeometryChange();
    d->cache = d->target->untransformed_bounding_rect(d->target->document()->current_time());
    for ( const auto& h : d->handles )
    {
        d->set_pos(h);
    }
}

void model::graphics::TransformGraphicsItem::update_transform()
{
    d->tranform_matrix = d->transform->transform_matrix();
    d->tranform_matrix_inv = d->tranform_matrix.inverted();
    setTransform(d->tranform_matrix);
    d->set_pos(d->handles[Private::Rot]);
    d->set_pos(d->handles[Private::Anchor]);
}


void model::graphics::TransformGraphicsItem::drag_tl(const QPointF& p)
{
    if ( d->cache.width() != 0 && d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        d->transform->scale.set_undoable(QVector2D(
            d->find_scale(d->cache.left(), sp.x(), d->cache.width()),
            d->find_scale(d->cache.top(), sp.y(), d->cache.height())
        ));
    }
}

void model::graphics::TransformGraphicsItem::drag_tr(const QPointF& p)
{
    if ( d->cache.width() != 0 && d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        d->transform->scale.set_undoable(QVector2D(
            d->find_scale(sp.x(), d->cache.right(), d->cache.width()),
            d->find_scale(d->cache.top(), sp.y(), d->cache.height())
        ));
    }
}

void model::graphics::TransformGraphicsItem::drag_br(const QPointF& p)
{
    if ( d->cache.width() != 0 && d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        d->transform->scale.set_undoable(QVector2D(
            d->find_scale(sp.x(), d->cache.right(), d->cache.width()),
            d->find_scale(sp.y(), d->cache.bottom(), d->cache.height())
        ));
    }
}

void model::graphics::TransformGraphicsItem::drag_bl(const QPointF& p)
{
    if ( d->cache.width() != 0 && d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        d->transform->scale.set_undoable(QVector2D(
            d->find_scale(d->cache.left(), sp.x(), d->cache.width()),
            d->find_scale(sp.y(), d->cache.bottom(), d->cache.height())
        ));
    }
}

void model::graphics::TransformGraphicsItem::drag_t(const QPointF& p)
{
    if ( d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        auto old = d->transform->scale.get();
        qreal new_scale = d->find_scale(d->cache.top(), sp.y(), d->cache.height());
        d->transform->scale.set_undoable(QVector2D(old.x(), new_scale));
    }
}

void model::graphics::TransformGraphicsItem::drag_b(const QPointF& p)
{
    if ( d->cache.height() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        auto old = d->transform->scale.get();
        qreal new_scale = d->find_scale(sp.y(), d->cache.bottom(), d->cache.height());
        d->transform->scale.set_undoable(QVector2D(old.x(), new_scale));
    }
}

void model::graphics::TransformGraphicsItem::drag_l(const QPointF& p)
{
    if ( d->cache.width() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        auto old = d->transform->scale.get();
        qreal new_scale = d->find_scale(d->cache.left(), sp.x(), d->cache.width());
        d->transform->scale.set_undoable(QVector2D(new_scale, old.y()));
    }
}

void model::graphics::TransformGraphicsItem::drag_r(const QPointF& p)
{
    if ( d->cache.width() != 0 )
    {
        QTransform scene_to_parent = parentItem()->sceneTransform().inverted();
        QPointF sp = scene_to_parent.map(sceneTransform().map(p));

        auto old = d->transform->scale.get();
        qreal new_scale = d->find_scale(sp.x(), d->cache.right(), d->cache.width());
        d->transform->scale.set_undoable(QVector2D(new_scale, old.y()));
    }
}

void model::graphics::TransformGraphicsItem::drag_a(const QPointF& p)
{
    QPointF anchor = p;
    QPointF anchor_old = d->transform->anchor_point.get();

    QPointF p1 = sceneTransform().map(QPointF(0, 0));
    d->transform->anchor_point.set(anchor);
    QPointF p2 = sceneTransform().map(QPointF(0, 0));
    QPointF pos = d->transform->position.get() - p2 + p1;
    d->transform->anchor_point.set(anchor_old);
    d->target->document()->undo_stack().push(new command::SetMultipleProperties(
        tr("Drag anchor point"),
        false,
        {&d->transform->anchor_point, &d->transform->position},
        anchor,
        pos
    ));
}

void model::graphics::TransformGraphicsItem::drag_rot(const QPointF& p)
{
    QPointF diff_old = d->get_rot() - d->transform->anchor_point.get();
    QVector2D scale = d->transform->scale.get();
    qreal angle_to_rot_handle = std::atan2(diff_old.y() * scale.y(), diff_old.x() * scale.x());

    QPointF p_new = d->tranform_matrix.map(p);
    QPointF ap = d->tranform_matrix.map(d->transform->anchor_point.get());
    QPointF diff_new = p_new - ap;
    qreal angle_new = std::atan2(diff_new.y(), diff_new.x());
    qreal angle = angle_new - angle_to_rot_handle;
    d->transform->rotation.set_undoable(qRadiansToDegrees(angle));
}

void model::graphics::TransformGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* opt, QWidget*)
{
    painter->save();
    QPen pen(opt->palette.color(QPalette::Highlight), 1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->drawRect(d->cache);
    painter->drawLine(d->handles[Private::Top].handle->pos(), d->handles[Private::Rot].handle->pos());
    painter->restore();
}

QRectF model::graphics::TransformGraphicsItem::boundingRect() const
{
    return d->cache;
}

