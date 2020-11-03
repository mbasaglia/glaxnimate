#include "timing_dialog.hpp"
#include "ui_timing_dialog.h"

#include <limits>

#include <QEvent>

#include "command/undo_macro_guard.hpp"
#include "model/simple_visitor.hpp"

class TimingDialog::Private
{
public:
    model::Document* document;
    bool changed = false;
    Ui::TimingDialog ui;
};

TimingDialog::TimingDialog(model::Document* document, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->document = document;
    d->ui.setupUi(this);

    d->ui.spin_seconds->setMaximum(std::numeric_limits<float>::max());
    d->ui.spin_frames->setMaximum(std::numeric_limits<int>::max());

    d->ui.spin_seconds->blockSignals(true);
    d->ui.spin_fps->blockSignals(true);
    d->ui.spin_frames->blockSignals(true);

    float fps = document->main()->fps.get();
    d->ui.spin_fps->setValue(fps);
    float n_frames = d->document->main()->animation->last_frame.get() - d->document->main()->animation->first_frame.get();
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
        command::UndoMacroGuard guard(tr("Change Animation Properties"), d->document);

        qreal last_frame = d->document->main()->animation->first_frame.get() + d->ui.spin_frames->value();
        d->document->main()->animation->last_frame.set_undoable(last_frame);

        model::simple_visit<model::Layer>(d->document->main(), true, [last_frame](model::Layer* layer){
            if ( layer->animation->last_frame.get() > last_frame )
                layer->animation->last_frame.set(last_frame);
        });

        d->document->main()->fps.set(d->ui.spin_fps->value());
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


