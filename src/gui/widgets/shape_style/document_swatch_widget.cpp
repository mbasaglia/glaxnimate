#include "document_swatch_widget.hpp"
#include "ui_document_swatch_widget.h"

#include <QMenu>
#include <QInputDialog>

#include <QtColorWidgets/color_palette_model.hpp>
#include <QtColorWidgets/ColorDialog>

#include "model/defs/defs.hpp"
#include "model/document.hpp"
#include "command/object_list_commands.hpp"
#include "command/animation_commands.hpp"
#include "utils/pseudo_mutex.hpp"
#include "model/visitor.hpp"
#include "model/shapes/styler.hpp"
#include "model/defs/named_color.hpp"

class DocumentSwatchWidget::Private
{
public:
    Ui::DocumentSwatchWidget ui;
    model::Document* document = nullptr;
    utils::PseudoMutex updating_swatch;
    color_widgets::ColorPaletteModel* palette_model = nullptr;
    QPersistentModelIndex palette_index;


    class FetchColorVisitor : public model::Visitor
    {
    private:
        void on_visit(model::Document * doc) override
        {
            defs = doc->defs();
            doc->undo_stack().beginMacro(tr("Gather Document Swatch"));
            for ( const auto& color : defs->colors )
            {
                if ( !color->color.animated() )
                {
                    QColor c = color->color.get();
                    colors[c.name(QColor::HexArgb)] = color.get();
                }
            }
        }

        void on_visit_end(model::Document * document) override
        {
            document->undo_stack().endMacro();
        }

        void on_visit(model::DocumentNode * node) override
        {
            if ( auto sty = qobject_cast<model::Styler*>(node) )
            {
                if ( !sty->use.get() && !sty->color.animated() )
                {
                    QString color_name = sty->color.get().name(QColor::HexArgb);
                    auto it = colors.find(color_name);
                    model::NamedColor* def = nullptr;
                    if ( it == colors.end() )
                    {
                        auto ptr = std::make_unique<model::NamedColor>(node->document());
                        ptr->color.set(sty->color.get());
                        def = ptr.get();
                        node->push_command(new command::AddObject<model::NamedColor>(
                            &defs->colors,
                            std::move(ptr),
                            defs->colors.size()
                        ));
                        colors[color_name] = def;
                    }
                    else
                    {
                        def = it->second;
                    }

                    sty->use.set_undoable(QVariant::fromValue(def));
                }
            }
        }

        std::map<QString, model::NamedColor*> colors;
        model::Defs* defs;
    };
};

DocumentSwatchWidget::DocumentSwatchWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    auto palette = &d->ui.swatch->palette();
    connect(palette, &color_widgets::ColorPalette::colorAdded, this, &DocumentSwatchWidget::swatch_palette_color_added);
    connect(palette, &color_widgets::ColorPalette::colorRemoved, this, &DocumentSwatchWidget::swatch_palette_color_removed);
    connect(palette, &color_widgets::ColorPalette::colorChanged, this, &DocumentSwatchWidget::swatch_palette_color_changed);

    QMenu* menu = new QMenu(this);
    menu->addAction(d->ui.action_generate);
    menu->addAction(d->ui.action_open);
    menu->addAction(d->ui.action_save);
    d->ui.button_extra->setMenu(menu);
    connect(d->ui.action_generate, &QAction::triggered, this, &DocumentSwatchWidget::generate);
    connect(d->ui.action_open, &QAction::triggered, this, &DocumentSwatchWidget::open);
    connect(d->ui.action_save, &QAction::triggered, this, &DocumentSwatchWidget::save);
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

        palette->setName("");

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
    if ( d->document )
    {
        auto def = index != -1 ? &d->document->defs()->colors[index] : nullptr;

        if ( mod & Qt::ShiftModifier )
            emit secondary_color_def(def);
        else
            emit current_color_def(def);
    }
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

void DocumentSwatchWidget::generate()
{
    Private::FetchColorVisitor().visit(d->document);
}

void DocumentSwatchWidget::open()
{
}

void DocumentSwatchWidget::save()
{
    if ( d->ui.swatch->palette().name().isEmpty() )
        d->ui.swatch->palette().setName(d->document->main_composition()->name.get());

    if ( !d->palette_index.isValid() )
    {
        d->palette_model->addPalette(d->ui.swatch->palette(), true);
        d->palette_index = d->palette_model->index(d->palette_model->count() - 1, 0);
    }
    else
    {
        d->palette_model->updatePalette(d->palette_index.row(), d->ui.swatch->palette(), true);
    }
}

void DocumentSwatchWidget::set_palette_model(color_widgets::ColorPaletteModel* palette_model)
{
    d->palette_model = palette_model;
}

void DocumentSwatchWidget::swatch_menu ( int index )
{
    QMenu menu;

    if ( index == -1 )
    {
        menu.addSection(tr("Unlink Color"));

        menu.addAction(
            QIcon::fromTheme("format-fill-color"),
            tr("Unlink fill"),
            this,
            [this]{
                emit current_color_def(nullptr);
            }
        );

        menu.addAction(
            QIcon::fromTheme("format-stroke-color"),
            tr("Unlink stroke"),
            this,
            [this]{
                emit secondary_color_def(nullptr);
            }
        );
    }
    else
    {
        model::NamedColor* item = &d->document->defs()->colors[index];
        menu.addSection(item->object_name());

        menu.addAction(
            QIcon::fromTheme("edit-rename"),
            tr("Rename..."),
            this,
            [item, this]{
                bool ok = false;
                QString name = QInputDialog::getText(this, tr("Rename Swatch Color"), tr("Name"), QLineEdit::Normal, item->type_name_human(), &ok);
                if ( ok )
                    item->name.set_undoable(name);
            }
        );

        menu.addAction(
            QIcon::fromTheme("color-management"),
            tr("Edit Color..."),
            this,
            [item, this]{
                color_widgets::ColorDialog dialog(this);
                QColor old = item->color.get();
                dialog.setColor(old);
                connect(&dialog, &color_widgets::ColorDialog::colorChanged, &item->color, &model::AnimatedProperty<QColor>::set);
                auto result = dialog.exec();
                item->color.set(old);
                if ( result == QDialog::Accepted )
                    item->color.set_undoable(dialog.color());
            }
        );

        menu.addSeparator();

        menu.addAction(
            QIcon::fromTheme("list-remove"),
            tr("Remove"),
            this,
            [index, this]{
                d->ui.swatch->palette().eraseColor(index);
            }
        );

        menu.addAction(
            QIcon::fromTheme("edit-duplicate"),
            tr("Duplicate"),
            this,
            [item, index, this]{
                auto clone = item->clone_covariant();
                item->push_command(new command::AddObject(
                    &d->document->defs()->colors,
                    std::move(clone),
                    index+1
                ));
            }
        );

        menu.addSeparator();

        menu.addAction(
            QIcon::fromTheme("format-fill-color"),
            tr("Set as fill"),
            this,
            [item, this]{
                emit current_color_def(item);
            }
        );

        menu.addAction(
            QIcon::fromTheme("format-stroke-color"),
            tr("Set as stroke"),
            this,
            [item, this]{
                emit secondary_color_def(item);
            }
        );
    }

    menu.exec(QCursor::pos());

}
