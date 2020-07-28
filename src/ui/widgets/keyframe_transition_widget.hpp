#pragma once

#include <memory>
#include <QWidget>
#include "model/animation/keyframe_transition.hpp"

class KeyframeTransitionWidget : public QWidget
{
    Q_OBJECT

public:
    KeyframeTransitionWidget(QWidget* parent = nullptr);
    ~KeyframeTransitionWidget();

    void set_target(model::KeyframeTransition* kft);

protected:
    void paintEvent(QPaintEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;
    void focusOutEvent(QFocusEvent * event) override;
    void leaveEvent(QEvent * event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};
