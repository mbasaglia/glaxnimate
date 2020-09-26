#include "document_swatch_widget.hpp"
#include "ui_document_swatch_widget.h"


#include "model/defs/defs.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"
#include "command/animation_commands.hpp"
#include "utils/pseudo_mutex.hpp"

class DocumentSwatchWidget::Private
{
public:
    Ui::DocumentSwatchWidget ui;
    model::Document* document = nullptr;
    utils::PseudoMutex updating_swatch;
};

DocumentSwatchWidget::DocumentSwatchWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    auto palette = &d->ui.swatch->palette();
    connect(palette, &color_widgets::ColorPalette::colorAdded, this, &DocumentSwatchWidget::swatch_palette_color_added);
    connect(palette, &color_widgets::ColorPalette::colorRemoved, this, &DocumentSwatchWidget::swatch_palette_color_removed);
    connect(palette, &color_widgets::ColorPalette::colorChanged, this, &DocumentSwatchWidget::swatch_palette_color_changed);
}

DocumentSwatchWidget::~DocumentSwatchWidget() = default;

void DocumentSwatchWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void DocumentSwatchWidget::set_document(model::Document* document)
{
    auto palette = &d->ui.swatch->palette();

    if ( d->document )
    {
        disconnect(d->document->defs(), nullptr, this, nullptr);
    }

    d->document = document;

    if ( d->document )
    {
        auto l = d->updating_swatch.get_lock();
        palette->setColors(QVector<QColor>{});
        for ( const auto& col : d->document->defs()->colors )
            palette->appendColor(col->color.get(), col->name.get());

        connect(d->document->defs(), &model::Defs::color_added, this, &DocumentSwatchWidget::swatch_doc_color_added);
        connect(d->document->defs(), &model::Defs::color_removed, this, &DocumentSwatchWidget::swatch_doc_color_removed);
        connect(d->document->defs(), &model::Defs::color_changed, this, &DocumentSwatchWidget::swatch_doc_color_changed);
    }
}


void DocumentSwatchWidget::swatch_palette_color_added(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {
        auto defs = d->document->defs();
        auto palette = &d->ui.swatch->palette();

        auto col = std::make_unique<model::NamedColor>(d->document);
        col->color.set(palette->colorAt(index));
        col->name.set(palette->nameAt(index));
        defs->push_command(new command::AddObject<model::NamedColor>(&defs->colors, std::move(col), index));
    }
}

void DocumentSwatchWidget::swatch_palette_color_removed(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {
        auto defs = d->document->defs();

        if ( defs->colors.valid_index(index) )
            defs->push_command(new command::RemoveObject<model::NamedColor>(index, &defs->colors));
    }
}

void DocumentSwatchWidget::swatch_palette_color_changed(int index)
{
    if ( !d->document )
        return;

    if ( auto l = d->updating_swatch.get_lock() )
    {

        auto defs = d->document->defs();
        auto palette = &d->ui.swatch->palette();

        if ( !defs->colors.valid_index(index) )
            return;

        d->document->undo_stack().beginMacro(tr("Modify Palette Color"));
        auto color = &defs->colors[index];
        color->name.set_undoable(palette->nameAt(index));
        color->color.set_undoable(palette->colorAt(index));
        d->document->undo_stack().endMacro();
    }
}

void DocumentSwatchWidget::swatch_add()
{
    emit needs_new_color();
}

void DocumentSwatchWidget::add_new_color(const QColor& color)
{
    d->ui.swatch->palette().appendColor(color);
    swatch_link(d->ui.swatch->palette().count() - 1, {});
}


void DocumentSwatchWidget::swatch_link(int index, Qt::KeyboardModifiers mod)
{
    if ( d->document && index != -1 )
    {
        if ( mod & Qt::ShiftModifier )
            emit secondary_color_def(&d->document->defs()->colors[index]);
        else
            emit current_color_def(&d->document->defs()->colors[index]);
    }
}

void DocumentSwatchWidget::swatch_unlink()
{
    emit current_color_def(nullptr);
    emit secondary_color_def(nullptr);
}


void DocumentSwatchWidget::swatch_doc_color_added(int position, model::NamedColor* color)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().insertColor(position, color->color.get(), color->name.get());
    }
}

void DocumentSwatchWidget::swatch_doc_color_removed(int pos)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().eraseColor(pos);
    }
}

void DocumentSwatchWidget::swatch_doc_color_changed(int position, model::NamedColor* color)
{
    if ( auto l = d->updating_swatch.get_lock() )
    {
        d->ui.swatch->palette().setColorAt(position, color->color.get(), color->name.get());
    }
}

model::NamedColor * DocumentSwatchWidget::current_color() const
{
    if ( !d->document )
        return nullptr;

    int index = d->ui.swatch->selected();
    if ( index == -1 )
        return nullptr;

    return &d->document->defs()->colors[index];
}
