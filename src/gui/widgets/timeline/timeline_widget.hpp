#pragma once

#include <memory>

#include <QGraphicsView>

#include "model/document.hpp"
#include "item_models/property_model_full.hpp"


class TimelineWidget : public QGraphicsView
{
    Q_OBJECT
    
public:
    TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();
    
//     void clear();

    void set_model(item_models::PropertyModelFull* model);

    int row_height() const;
    void set_row_height(int w);
    int header_height() const;

    qreal highlighted_time() const;
    void set_highlighted_time(int time);
    void keep_highlight();
    
    void select(const QModelIndex& index);
    
    item_models::PropertyModelBase::Item item_at(const QPoint& viewport_pos);
    std::pair<model::KeyframeBase*, model::KeyframeBase*> keyframe_at(const QPoint& viewport_pos);

    void expand(const QModelIndex& obj);
    void collapse(const QModelIndex& obj);
    void set_document(model::Document* document);

public slots:
    void update_timeline_start(model::FrameTime start);
    void update_timeline_end(model::FrameTime end);
    void reset_view();
/*
private slots:
    void update_layer_start(model::FrameTime start);
    void update_layer_end(model::FrameTime en);
*/
private:
    void model_rows_added(const QModelIndex& parent, int first, int last);
    void model_rows_removed(const QModelIndex& parent, int first, int last);
    void model_rows_moved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void model_reset();
    void on_item_removed(quintptr id);

protected:    
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;
    void paintEvent(QPaintEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void scrollContentsBy(int dx, int dy) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void leaveEvent(QEvent * event) override;
    void enterEvent(QEvent * event) override;
    void keyPressEvent(QKeyEvent * event) override;
    
signals:
    void frame_clicked(int frame);
    void line_clicked(quintptr id);
    
private:
    class Private;
    
    std::unique_ptr<Private> d;
};
