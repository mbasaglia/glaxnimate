/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef KEYFRAMEEDITORWIDGET_H
#define KEYFRAMEEDITORWIDGET_H

#include <QWidget>
#include <memory>
#include "model/animation/keyframe_transition.hpp"

namespace glaxnimate::gui {

namespace Ui {
class KeyframeEditorWidget;
}

class KeyframeEditorWidget : public QWidget
{
    Q_OBJECT

public:
    KeyframeEditorWidget(QWidget* parent = nullptr);
    ~KeyframeEditorWidget();

    void set_target(model::KeyframeTransition* kft);

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void preset_before(int index);
    void preset_after(int index);
    void update_before(model::KeyframeTransition::Descriptive v);
    void update_after(model::KeyframeTransition::Descriptive v);

private:
    std::unique_ptr<Ui::KeyframeEditorWidget> d;
};

} // namespace glaxnimate::gui

#endif // KEYFRAMEEDITORWIDGET_H
