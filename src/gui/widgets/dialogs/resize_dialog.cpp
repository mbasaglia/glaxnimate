#include "resize_dialog.hpp"
#include "ui_resize_dialog.h"

#include <QEvent>

#include "command/property_commands.hpp"
#include "command/shape_commands.hpp"
#include "command/undo_macro_guard.hpp"

class ResizeDialog::Private
{
public:
    Ui::ResizeDialog ui;
    double ratio = 1;

    void resize(model::Document* doc)
    {
        doc->push_command(new command::SetMultipleProperties(
            tr("Resize Document"),
            true,
            {&doc->main()->width, &doc->main()->height},
            ui.spin_width->value(),
            ui.spin_height->value()
        ));
    }
};

ResizeDialog::ResizeDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ResizeDialog::~ResizeDialog() = default;

void ResizeDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ResizeDialog::width_changed(int w)
{
    if ( d->ui.check_aspect->isChecked() )
        d->ui.spin_height->setValue(w / d->ratio);
}


void ResizeDialog::height_changed(int h)
{
    if ( d->ui.check_aspect->isChecked() )
        d->ui.spin_width->setValue(h * d->ratio);
}

void ResizeDialog::resize_document(model::Document* doc)
{
    auto comp = doc->main();
    d->ui.spin_width->setValue(comp->width.get());
    d->ui.spin_height->setValue(comp->height.get());
    d->ratio = double(comp->width.get()) / comp->height.get();
    d->ui.check_aspect->setChecked(true);

    if ( exec() == QDialog::Rejected )
        return;

    if ( d->ui.check_scale_layers->isChecked() && !comp->shapes.empty() )
    {
        command::UndoMacroGuard macro(tr("Resize Document"), doc);

        auto nl = std::make_unique<model::Layer>(doc);
        model::Layer* layer = nl.get();
        doc->set_best_name(layer, tr("Resize"));
        doc->push_command(new command::AddShape(&comp->shapes, std::move(nl), comp->shapes.size()));
        while ( comp->shapes[0] != layer )
            doc->push_command(new command::MoveShape(comp->shapes[0], &comp->shapes, &layer->shapes, layer->shapes.size()));

        qreal scale_w = comp->width.get() != 0 ? double(d->ui.spin_width->value()) / comp->width.get() : 1;
        qreal scale_h = comp->height.get() != 0 ? double(d->ui.spin_height->value()) / comp->height.get() : 1;
        if ( d->ui.check_layer_ratio->isChecked() )
            scale_h = scale_w = std::min(scale_h, scale_w);

        layer->transform.get()->scale.set_undoable(QVector2D(scale_w, scale_h));

        d->resize(doc);
    }
    else
    {
        d->resize(doc);
    }
}

void ResizeDialog::lock_changed(bool locked)
{
    d->ui.check_aspect->setIcon(QIcon::fromTheme(locked ? "lock" : "unlock"));
    width_changed(d->ui.spin_width->value());
}
