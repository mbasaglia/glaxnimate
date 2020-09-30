#include "resize_dialog.hpp"
#include "ui_resize_dialog.h"

#include <QEvent>

#include "command/property_commands.hpp"
#include "command/layer_commands.hpp"
#include "model/layers/empty_layer.hpp"

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

    if ( d->ui.check_scale_layers->isChecked() && !comp->layers.empty() )
    {
        doc->undo_stack().beginMacro(tr("Resize Document"));

        auto range = comp->top_level();
        auto it = range.begin();
        model::Layer* layer = *it;
        if ( ++it != range.end() || layer->transform.get()->position.get() != QPointF(0, 0) )
        {
            auto nl = std::make_unique<model::EmptyLayer>(doc, comp);
            layer = nl.get();
            doc->set_best_name(layer, tr("Resize"));
            for ( auto lay : range )
                lay->parent.set_undoable(QVariant::fromValue(layer));
            doc->push_command(new command::AddLayer(comp, std::move(nl), comp->layers.size()));
        }

        qreal scale_w = comp->width.get() != 0 ? double(d->ui.spin_width->value()) / comp->width.get() : 1;
        qreal scale_h = comp->height.get() != 0 ? double(d->ui.spin_height->value()) / comp->height.get() : 1;
        if ( d->ui.check_layer_ratio->isChecked() )
            scale_h = scale_w = std::min(scale_h, scale_w);

        layer->transform.get()->scale.set_undoable(QVector2D(scale_w, scale_h));

        d->resize(doc);
        doc->undo_stack().endMacro();
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
