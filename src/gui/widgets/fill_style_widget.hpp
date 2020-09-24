#pragma once
#include "color_selector.hpp"
#include "model/shapes/fill.hpp"
#include "model/defs/named_color.hpp"
#include "model/document.hpp"

class FillStyleWidget : public ColorSelector
{
public:
    FillStyleWidget(QWidget* parent = nullptr)
    : ColorSelector(parent)
    {
        connect(this, &ColorSelector::current_color_changed,
                this, &FillStyleWidget::set_target_color);
        connect(this, &ColorSelector::current_color_committed,
                this, &FillStyleWidget::commit_target_color);
        connect(this, &ColorSelector::current_color_def,
                this, &FillStyleWidget::set_target_def);
    }


    void set_shape(model::Fill* target)
    {
        if ( this->target )
        {
            disconnect(this->target, &model::Object::property_changed,
                       this, &FillStyleWidget::property_changed);
        }

        this->target = target;

        if ( target )
        {
            update_from_target();
            connect(target, &model::Object::property_changed,
                    this, &FillStyleWidget::property_changed);
        }
    }

private:
    void update_from_target()
    {
        updating = true;
        set_current_color(target->color.get());
        emit current_color_changed(target->color.get());
        update();
        updating = false;
    }

private slots:
    void set_target_color(const QColor& color)
    {
        if ( !target || updating )
            return;

        target->document()->undo_stack().beginMacro(tr("Update Fill Color"));
        target->color.set_undoable(color, false);

        if (  auto named_color = qobject_cast<model::NamedColor*>(target->use.get()) )
            named_color->color.set(color);

        target->document()->undo_stack().endMacro();
    }

    void commit_target_color()
    {
        if ( !target || updating )
            return;
        target->color.set_undoable(target->color.get(), true);
    }

    void property_changed(const model::BaseProperty* prop)
    {
        if ( prop == &target->color )
        {
            update_from_target();
        }
    }

    void set_target_def(model::BrushStyle* def)
    {
        if ( !target || updating )
            return;
        target->use.set_undoable(QVariant::fromValue(def));
    }

private:
    model::Fill* target = nullptr;
    bool updating = false;
};

