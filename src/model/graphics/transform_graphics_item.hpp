#pragma once

#include "model/graphics/handle.hpp"
#include "model/transform.hpp"
#include "model/document_node.hpp"


namespace model::graphics {

class TransformGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public:
    TransformGraphicsItem(Transform* transform, DocumentNode* target, QGraphicsItem* parent);
    ~TransformGraphicsItem();

    QRectF boundingRect() const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private slots:
    void drag_tl(const QPointF& p);
    void drag_tr(const QPointF& p);
    void drag_br(const QPointF& p);
    void drag_bl(const QPointF& p);
    void drag_t(const QPointF& p);
    void drag_b(const QPointF& p);
    void drag_l(const QPointF& p);
    void drag_r(const QPointF& p);
    void drag_a(const QPointF& p);
    void drag_rot(const QPointF& p);

    void update_handles();
    void update_transform();


private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model::graphics
