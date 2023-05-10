/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "timing_dialog.hpp"
#include "ui_timing_dialog.h"

#include <limits>

#include <QEvent>

#include "command/undo_macro_guard.hpp"
#include "command/animation_commands.hpp"
#include "model/simple_visitor.hpp"
#include "model/assets/composition.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class TimingDialog::Private
{
public:
    model::Composition* comp;
    bool changed = false;
    Ui::TimingDialog ui;
};

TimingDialog::TimingDialog(model::Composition* comp, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->comp = comp;
    d->ui.setupUi(this);

    d->ui.spin_seconds->setMaximum(std::numeric_limits<float>::max());
    d->ui.spin_frames->setMaximum(std::numeric_limits<int>::max());

    d->ui.spin_seconds->blockSignals(true);
    d->ui.spin_fps->blockSignals(true);
    d->ui.spin_frames->blockSignals(true);

    float fps = comp->fps.get();
    d->ui.spin_fps->setValue(fps);
    float n_frames = d->comp->animation->last_frame.get() - d->comp->animation->first_frame.get();
    if ( fps != 0 )
        d->ui.spin_seconds->setValue(n_frames / fps);

    d->ui.spin_frames->setValue(n_frames);


    d->ui.spin_seconds->blockSignals(false);
    d->ui.spin_fps->blockSignals(false);
    d->ui.spin_frames->blockSignals(false);
}

TimingDialog::~TimingDialog() = default;

void TimingDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void TimingDialog::btn_clicked(QAbstractButton* button)
{
    auto role = d->ui.button_box->buttonRole(button);

    if ( role == QDialogButtonBox::RejectRole )
    {
        reject();
        return;
    }

    if ( d->changed )
    {

        qreal last_frame = d->comp->animation->first_frame.get() + d->ui.spin_frames->value();
        command::UndoMacroGuard guard(tr("Change Animation Properties"), d->comp->document());

        if ( d->ui.check_layer_scale->isChecked() )
        {
            if ( last_frame != 0 )
            {
                qreal multiplier = last_frame / d->comp->animation->last_frame.get();
                d->comp->push_command(new command::StretchTimeCommand(d->comp, multiplier));
            }
        }
        else
        {
            d->comp->animation->last_frame.set_undoable(last_frame);

            if ( d->ui.check_layer_trim->isChecked() )
            {
                model::simple_visit<model::Layer>(d->comp, true, [last_frame](model::Layer* layer){
                    if ( layer->animation->last_frame.get() > last_frame )
                        layer->animation->last_frame.set(last_frame);
                });
            }
        }

        d->comp->fps.set(d->ui.spin_fps->value());
    }

    if ( role == QDialogButtonBox::AcceptRole )
        accept();
}

void TimingDialog::changed_fps(double fps)
{
    d->ui.spin_seconds->setValue(d->ui.spin_frames->value() / fps);
    d->changed = true;
}

void TimingDialog::changed_frames(int f)
{
    d->ui.spin_seconds->setValue(f / d->ui.spin_fps->value());
    d->changed = true;
}

void TimingDialog::changed_seconds(double s)
{
    d->ui.spin_frames->setValue(qRound(d->ui.spin_fps->value() * s));
    d->changed = true;
}


