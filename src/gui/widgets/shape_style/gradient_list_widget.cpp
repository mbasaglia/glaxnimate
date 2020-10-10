#include "gradient_list_widget.hpp"
#include "ui_gradient_list_widget.h"

#include <QEvent>

#include <QtColorWidgets/gradient_delegate.hpp>

#include "model/document.hpp"
#include "model/defs/defs.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"

#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"
#include "command/property_commands.hpp"

#include "item_models/gradient_list_model.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

class GradientListWidget::Private
{
public:
    Ui::GradientListWidget ui;
    item_models::GradientListModel model;
    model::Document* document = nullptr;
    GlaxnimateWindow* window = nullptr;
    model::Fill* fill = nullptr;
    model::Stroke* stroke = nullptr;
    color_widgets::GradientDelegate delegrate;

    model::GradientColors* current()
    {
        if ( !document )
            return nullptr;
        auto index = ui.list_view->currentIndex();
        if ( !index.isValid() )
            return nullptr;
        return document->defs()->gradient_colors[index.row()];
    }

    struct TypeButtonSlot
    {
        QToolButton* btn_other;
        GradientListWidget* widget;
        model::Gradient::Type gradient_type;
        bool secondary;

        void operator() (bool checked)
        {
            if ( !checked )
            {
                widget->d->clear_gradient(secondary);
            }
            else
            {
                btn_other->setChecked(false);
                widget->d->set_gradient(secondary, gradient_type);
            }
        }
    };

    TypeButtonSlot slot_radial(GradientListWidget* widget, bool secondary)
    {
        return {
            secondary ? ui.btn_stroke_linear : ui.btn_fill_linear,
            widget,
            model::Gradient::Radial,
            secondary
        };
    }

    TypeButtonSlot slot_linear(GradientListWidget* widget, bool secondary)
    {
        return {
            secondary ? ui.btn_stroke_radial : ui.btn_fill_radial,
            widget,
            model::Gradient::Linear,
            secondary
        };
    }


    void set_gradient(bool secondary, model::Gradient::Type gradient_type)
    {

        model::Styler* styler = secondary ? (model::Styler*)stroke : (model::Styler*)fill;
        if ( !styler )
            return;

        model::GradientColors* colors = current();
        if ( !colors )
        {
            if ( document->defs()->gradient_colors.empty() )
                add_gradient();
            else
                ui.list_view->setCurrentIndex(model.gradient_to_index(document->defs()->gradient_colors.back()));

            colors = current();
        }


        command::UndoMacroGuard macro(tr("Set %1 Gradient").arg(model::Gradient::gradient_type_name(gradient_type)), document);

        model::Gradient* old = nullptr;

        if ( styler->use.get() )
        {
            old = styler->use->cast<model::Gradient>();

            if ( old )
            {
                document->push_command(new command::SetPropertyValue(
                    &old->type,
                    QVariant::fromValue(gradient_type)
                ));

                document->push_command(new command::SetPropertyValue(
                    &old->colors,
                    QVariant::fromValue(colors)
                ));

                return;
            }
        }

        auto shape_element = window->current_shape();
        QRectF bounds;

        if ( shape_element )
        {
            bounds = shape_element->local_bounding_rect(shape_element->time());
            if ( bounds.isNull() )
            {
                if ( auto parent = window->current_shape_container() )
                    bounds = parent->bounding_rect(shape_element->time());
            }
        }

        if ( bounds.isNull() )
            bounds = QRectF(QPointF(0, 0), document->size());


        auto grad = std::make_unique<model::Gradient>(document);
        grad->colors.set(colors);
        grad->type.set(gradient_type);

        if ( gradient_type == model::Gradient::Radial )
            grad->start_point.set(bounds.center());
        else
            grad->start_point.set(QPointF(bounds.left(), bounds.center().y()));

        grad->highlight.set(grad->start_point.get());
        grad->end_point.set(QPointF(bounds.right(), bounds.center().y()));

        model::Gradient* gradient = grad.get();
        document->push_command(new command::AddObject<model::Gradient>(
            &document->defs()->gradients,
            std::move(grad)
        ));


        styler->use.set_undoable(QVariant::fromValue(gradient));
        remove_old(old);

    }

    /// \todo Always check, maybe on use removed in the gradient/brush_style
    void remove_old(model::Gradient* old)
    {
        if ( old && old->users().empty() )
        {
            old->colors.set_undoable(QVariant::fromValue((model::GradientColors*)nullptr));
            document->push_command(new command::RemoveObject(
                old,
                &document->defs()->gradients
            ));
        }
    }

    void clear_gradient(bool secondary)
    {
        model::Styler* styler = secondary ? (model::Styler*)stroke : (model::Styler*)fill;
        if ( !styler )
            return;

        command::UndoMacroGuard macro(tr("Remove Gradient"), document);

        auto old = styler->use.get();

        styler->use.set_undoable(QVariant::fromValue((model::BrushStyle*)nullptr));

        if ( old )
            remove_old(old->cast<model::Gradient>());
    }

    void add_gradient()
    {
        if ( !document )
            return;

        auto ptr = std::make_unique<model::GradientColors>(document);
        auto raw = ptr.get();
        ptr->colors.set({{0, window->current_color()}, {1, window->secondary_color()}});
        document->push_command(new command::AddObject(
            &document->defs()->gradient_colors,
            std::move(ptr)
        ));

        clear_buttons();
        ui.list_view->setCurrentIndex(model.gradient_to_index(raw));
    }

    void clear_buttons()
    {
        ui.btn_fill_linear->setChecked(false);
        ui.btn_fill_radial->setChecked(false);
        ui.btn_stroke_linear->setChecked(false);
        ui.btn_stroke_radial->setChecked(false);
    }

    void set_targets(model::Fill* fill, model::Stroke* stroke)
    {
        this->fill = fill;
        this->stroke = stroke;

        auto gradient_fill = fill ? qobject_cast<model::Gradient*>(fill->use.get()) : nullptr;
        auto gradient_stroke = stroke ? qobject_cast<model::Gradient*>(stroke->use.get()) : nullptr;

        clear_buttons();

        model::GradientColors* colors_fill = gradient_fill ? gradient_fill->colors.get() : nullptr;
        model::GradientColors* colors_stroke = gradient_stroke ? gradient_stroke->colors.get() : nullptr;

        if ( !colors_fill && !colors_stroke )
            return;

        model::GradientColors* colors = colors_fill ? colors_fill : colors_stroke;

        ui.list_view->setCurrentIndex(model.gradient_to_index(colors));

        if ( colors_fill )
        {
            if ( gradient_fill->type.get() == model::Gradient::Radial )
                ui.btn_fill_radial->setChecked(true);
            else
                ui.btn_fill_linear->setChecked(true);
        }

        if ( colors_stroke == colors )
        {
            if ( gradient_stroke->type.get() == model::Gradient::Radial )
                ui.btn_stroke_radial->setChecked(true);
            else
                ui.btn_stroke_linear->setChecked(true);
        }
    }

    void delete_gradient()
    {
        model::GradientColors* colors = current();
        if ( !colors )
        {
            if ( document->defs()->gradient_colors.empty() )
                return;

            colors = document->defs()->gradient_colors.back();
        }

        document->push_command(new command::RemoveObject(
            colors,
            &document->defs()->gradient_colors
        ));
    }
};

GradientListWidget::GradientListWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->ui.list_view->setModel(&d->model);
    d->ui.list_view->horizontalHeader()->setSectionResizeMode(item_models::GradientListModel::Users, QHeaderView::ResizeToContents);
    d->ui.list_view->setItemDelegateForColumn(item_models::GradientListModel::Gradient, &d->delegrate);

    connect(d->ui.btn_new, &QAbstractButton::clicked, this, [this]{ d->add_gradient(); });
    connect(d->ui.btn_remove, &QAbstractButton::clicked, this, [this]{ d->delete_gradient(); });
    connect(d->ui.btn_fill_linear,   &QAbstractButton::clicked, this, d->slot_linear(this, false));
    connect(d->ui.btn_fill_radial,   &QAbstractButton::clicked, this, d->slot_radial(this, false));
    connect(d->ui.btn_stroke_linear, &QAbstractButton::clicked, this, d->slot_linear(this, true));
    connect(d->ui.btn_stroke_radial, &QAbstractButton::clicked, this, d->slot_radial(this, true));
}

GradientListWidget::~GradientListWidget() = default;

void GradientListWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void GradientListWidget::set_document(model::Document* document)
{
    d->document = document;
    d->fill = nullptr;
    d->stroke = nullptr;
    d->clear_buttons();

    if ( !document )
        d->model.set_defs(nullptr);
    else
        d->model.set_defs(document->defs());
}

void GradientListWidget::set_window(GlaxnimateWindow* window)
{
    d->window = window;
}


void GradientListWidget::set_targets(model::Fill* fill, model::Stroke* stroke)
{
    d->set_targets(fill, stroke);
}

