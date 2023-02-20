/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QWidget>
#include "model/animation/keyframe_transition.hpp"

namespace glaxnimate::gui {

class KeyframeTransitionWidget : public QWidget
{
    Q_OBJECT

public:
    KeyframeTransitionWidget(QWidget* parent = nullptr);
    ~KeyframeTransitionWidget();

    void set_target(model::KeyframeTransition* kft);
    model::KeyframeTransition* target() const;
    
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;
    void focusOutEvent(QFocusEvent * event) override;
    void leaveEvent(QEvent * event) override;

signals:
    void before_changed(model::KeyframeTransition::Descriptive v);
    void after_changed(model::KeyframeTransition::Descriptive v);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
