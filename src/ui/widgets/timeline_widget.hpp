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
    void add_container(model::AnimationContainer* cont);
    void add_animatable(model::AnimatableBase* anim);
    int row_height() const;
    void set_row_height(int w);
    
public slots:
    void update_timeline_start(model::FrameTime start);
    void update_timeline_end(model::FrameTime end);

protected:    
    void wheelEvent(QWheelEvent * event) override;
    void paintEvent(QPaintEvent * event) override;
    
private:
    class Private;
    
    std::unique_ptr<Private> d;
};
