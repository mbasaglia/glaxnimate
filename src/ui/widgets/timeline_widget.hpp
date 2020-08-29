#pragma once

#include <memory>

#include <QGraphicsView>

#include "model/document.hpp"


class TimelineWidget : public QGraphicsView
{
    Q_OBJECT
    
public:
    TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();
    
    void set_document(model::Document* document);
    void clear();
    void set_active(model::DocumentNode* node);
    void add_container(model::AnimationContainer* cont);
    void add_animatable(model::AnimatableBase* anim);
    int row_height() const;
    void set_row_height(int w);
    int header_height() const;
    
public slots:
    void update_timeline_start(model::FrameTime start);
    void update_timeline_end(model::FrameTime end);
    void reset_view();

protected:    
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;
    void paintEvent(QPaintEvent * event) override;
    void resizeEvent(QResizeEvent * event) override;
    void scrollContentsBy(int dx, int dy) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void leaveEvent(QEvent * event) override;
    
signals:
    void frame_clicked(int frame);
    
private slots:
    void kf_added(int pos, model::KeyframeBase* kf);
    void kf_removed(int pos);
    
private:
    class Private;
    
    std::unique_ptr<Private> d;
};
