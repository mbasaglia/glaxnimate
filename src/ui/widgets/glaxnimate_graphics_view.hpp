#pragma once

#include <QGraphicsView>

#include <memory>

class GlaxnimateGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    GlaxnimateGraphicsView(QWidget* parent = nullptr);
    ~GlaxnimateGraphicsView();

public:
    /**
     *  \brief Get the grobal zoom factor
     *
     *  \return A value representing the scaling factor, 1 = 100%
    */
    qreal get_zoom_factor() const;


    /// Overload QGraphicsView::translate
    void translate(const QPointF& d) { QGraphicsView::translate(d.x(),d.y()); }

    /**
     *  \brief Translate and resize sceneRect
     *
     *  Translate the scene, if the result is not contained within sceneRect,
     *  the sceneRect is expanded
     *
     *  \param delta Translation amount
     */
    void translate_view(const QPointF& delta);

public slots:
    /**
     * \brief Zoom view by factor
     *
     *  The zooming is performed relative to the current transformation
     *
     *  \param factor scaling factor ( 1 = don't zoom )
     */
    void zoom_view(qreal factor);
    /**
     * \brief Zoom view by factor
     *
     *  The zooming is performed relative to the current transformation
     *
     *  \param factor scaling factor ( 1 = don't zoom )
     *  \param anchor Point to keep stable (local coords)
     */
    void zoom_view(qreal factor, const QPoint& anchor);

    /**
     * \brief Set zoom factor
     *
     *  The zooming is performed absolutely
     *
     *  \param factor scaling factor ( 1 = no zoom )
     *  \param anchor Point to keep stable (local coords)
     */
    void set_zoom(qreal factor, const QPoint& anchor);

signals:
    /**
     *  \brief Emitted when zoom is changed
     *  \param percent Zoom percentage
     */
    void zoomed(qreal percent);

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
