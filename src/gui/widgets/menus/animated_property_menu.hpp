/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMenu>

#include "model/animation/animatable.hpp"

namespace glaxnimate::gui {

class SelectionManager;

class AnimatedPropertyMenu : public QMenu
{
    Q_OBJECT

public:
    AnimatedPropertyMenu(QWidget* parent = nullptr);
    ~AnimatedPropertyMenu();

    model::AnimatableBase* property() const;
    void set_property(model::AnimatableBase* property);
    void refresh_actions();
    void set_controller(SelectionManager* window);

    bool can_paste() const;

protected:
    void changeEvent ( QEvent* e ) override;

public slots:
    void paste_keyframe();
    void loop_keyframes();
    void follow_path();
    void remove_all_keyframes();
    void add_keyframe();
    void remove_keyframe();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
