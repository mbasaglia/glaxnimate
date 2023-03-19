/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "keyframe_editor_widget.hpp"
#include "ui_keyframe_editor_widget.h"
#include "app/application.hpp"
#include "keyframe_transition_data.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

KeyframeEditorWidget::KeyframeEditorWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Ui::KeyframeEditorWidget>())
{
    d->setupUi(this);


    for ( int i = 0; i < KeyframeTransitionData::count; i++ )
    {
        auto data = KeyframeTransitionData::from_index(i, KeyframeTransitionData::Start);
        d->combo_before->addItem(data.icon(), data.name);
        data = KeyframeTransitionData::from_index(i, KeyframeTransitionData::Finish);
        d->combo_after->addItem(data.icon(), data.name);
    }

    connect(d->bezier_editor, &KeyframeTransitionWidget::before_changed, this, &KeyframeEditorWidget::update_before);
    connect(d->bezier_editor, &KeyframeTransitionWidget::after_changed, this, &KeyframeEditorWidget::update_after);
}

KeyframeEditorWidget::~KeyframeEditorWidget() = default;

void KeyframeEditorWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void KeyframeEditorWidget::set_target(model::KeyframeTransition* kft)
{
    d->bezier_editor->set_target(kft);
}

void KeyframeEditorWidget::preset_after(int index)
{
    if ( !d->bezier_editor->target() )
        return;

    d->bezier_editor->target()->set_after_descriptive(model::KeyframeTransition::Descriptive(index));
    d->bezier_editor->update();
}

void KeyframeEditorWidget::preset_before(int index)
{
    if ( !d->bezier_editor->target() )
        return;

    d->bezier_editor->target()->set_before_descriptive(model::KeyframeTransition::Descriptive(index));
    d->bezier_editor->update();
}

void KeyframeEditorWidget::update_after(model::KeyframeTransition::Descriptive v)
{
    d->combo_after->setCurrentIndex(int(v));
}

void KeyframeEditorWidget::update_before(model::KeyframeTransition::Descriptive v)
{
    d->combo_before->setCurrentIndex(int(v));
}
