#include "stroke_style_widget.hpp"
#include "ui_stroke_style_widget.h"

#include <QPainter>
#include <QButtonGroup>
#include <QtMath>
#include <QStyleOptionFrame>

#include "app/settings/settings.hpp"

class StrokeStyleWidget::Private
{
public:
    Qt::PenCapStyle cap;
    Qt::PenJoinStyle join;
    Ui::StrokeStyleWidget ui;
    QButtonGroup group_cap;
    QButtonGroup group_join;
    bool dark_theme = false;
    QPalette::ColorRole background = QPalette::Base;
    model::Stroke* target = nullptr;
    bool updating = false;

    bool can_update_target()
    {
        return target && !target->docnode_locked_recursive();
    }

    void update_background(const QColor& color)
    {
        background = QPalette::Base;
        qreal val = color.valueF();
        qreal sat = color.saturationF();

        bool swap = val > 0.65 && sat < 0.5;
        if ( dark_theme )
            swap = !swap;

        if ( swap )
            background = QPalette::Text;
    }

    void update_from_target()
    {
        if ( target && !updating )
        {
            ui.spin_stroke_width->setValue(target->width.get());
            set_cap_style(target->cap.get());
            set_join_style(target->join.get());
            ui.spin_miter->setValue(target->miter_limit.get());
            ui.color_selector->set_current_color(target->color.get());
        }
    }

    void set_cap_style(model::Stroke::Cap cap)
    {
        this->cap = Qt::PenCapStyle(cap);

        switch ( cap )
        {
            case model::Stroke::ButtCap:
                ui.button_cap_butt->setChecked(true);
                break;
            case model::Stroke::RoundCap:
                ui.button_cap_round->setChecked(true);
                break;
            case model::Stroke::SquareCap:
                ui.button_cap_square->setChecked(true);
                break;
        }
    }

    void set_join_style(model::Stroke::Join join)
    {
        this->join = Qt::PenJoinStyle(join);

        switch ( join )
        {
            case model::Stroke::BevelJoin:
                ui.button_join_bevel->setChecked(true);
                break;
            case model::Stroke::RoundJoin:
                ui.button_join_round->setChecked(true);
                break;
            case model::Stroke::MiterJoin:
                ui.button_join_miter->setChecked(true);
                break;
        }
    }

    void set(model::BaseProperty& prop, const QVariant& value, bool commit)
    {
        updating = true;
        prop.set_undoable(value, commit);
        updating = false;
    }
};

StrokeStyleWidget::StrokeStyleWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->group_cap.addButton(d->ui.button_cap_butt);
    d->group_cap.addButton(d->ui.button_cap_round);
    d->group_cap.addButton(d->ui.button_cap_square);

    d->group_join.addButton(d->ui.button_join_bevel);
    d->group_join.addButton(d->ui.button_join_round);
    d->group_join.addButton(d->ui.button_join_miter);

    d->ui.spin_stroke_width->setValue(app::settings::get<qreal>("tools", "stroke_width"));
    d->ui.spin_miter->setValue(app::settings::get<qreal>("tools", "stroke_miter"));
    d->set_cap_style(app::settings::get<model::Stroke::Cap>("tools", "stroke_cap"));
    d->set_join_style(app::settings::get<model::Stroke::Join>("tools", "stroke_join"));

    d->ui.tab_widget->setTabEnabled(2, false);

    d->ui.color_selector->set_current_color(app::settings::get<QColor>("tools", "color_secondary"));
    d->ui.color_selector->hide_secondary();

    d->dark_theme = palette().window().color().valueF() < 0.5;
}

StrokeStyleWidget::~StrokeStyleWidget() = default;

void StrokeStyleWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void StrokeStyleWidget::paintEvent(QPaintEvent* event)
{

    QWidget::paintEvent(event);
    qreal stroke_width = d->ui.spin_stroke_width->value();
    const qreal frame_margin = 6;
    const qreal margin = frame_margin+stroke_width*M_SQRT2/2;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(pen_style());
    p.setBrush(Qt::NoBrush);

    QStyleOptionFrame panel;
    panel.initFrom(this);
    if ( isEnabled() )
        panel.state = QStyle::State_Enabled;
    panel.rect = d->ui.frame->geometry();
    panel.lineWidth = 2;
    panel.midLineWidth = 0;
    panel.state |= QStyle::State_Raised;
//     style()->drawPrimitive(QStyle::PE_Frame, &panel, &p, nullptr);
    QRect r = style()->subElementRect(QStyle::SE_FrameContents, &panel, nullptr);
    p.fillRect(r, palette().brush(d->background));

    QRectF draw_area = QRectF(d->ui.frame->geometry()).adjusted(margin, margin, -margin, -margin);
    QPolygonF poly;
    poly.push_back(draw_area.bottomLeft());
    poly.push_back(QPointF(draw_area.center().x(), draw_area.top()));
    poly.push_back(draw_area.bottomRight());

    p.drawPolyline(poly);
}

void StrokeStyleWidget::check_cap()
{
    if ( d->ui.button_cap_butt->isChecked() )
        d->cap = Qt::FlatCap;
    else if ( d->ui.button_cap_round->isChecked() )
        d->cap = Qt::RoundCap;
    else if ( d->ui.button_cap_square->isChecked() )
        d->cap = Qt::SquareCap;

    if ( d->can_update_target() )
        d->set(d->target->cap, int(d->cap), true);

    update();
}

void StrokeStyleWidget::check_join()
{
    if ( d->ui.button_join_bevel->isChecked() )
        d->join = Qt::BevelJoin;
    else if ( d->ui.button_join_round->isChecked() )
        d->join = Qt::RoundJoin;
    else if ( d->ui.button_join_miter->isChecked() )
        d->join = Qt::MiterJoin;

    if ( d->can_update_target() )
        d->set(d->target->join, int(d->join), true);

    update();
}

void StrokeStyleWidget::save_settings() const
{
    app::settings::set("tools", "stroke_width", d->ui.spin_stroke_width->value());
    app::settings::set("tools", "stroke_miter", d->ui.spin_miter->value());
    app::settings::set("tools", "stroke_cap", int(d->cap));
    app::settings::set("tools", "stroke_join", int(d->join));
}

QPen StrokeStyleWidget::pen_style() const
{
    QPen pen(d->ui.color_selector->current_color(), d->ui.spin_stroke_width->value());
    pen.setCapStyle(d->cap);
    pen.setJoinStyle(d->join);
    pen.setMiterLimit(d->ui.spin_miter->value());
    return pen;
}

void StrokeStyleWidget::set_color(const QColor& color)
{
    d->ui.color_selector->set_current_color(color);
    if ( d->can_update_target() )
        d->set(d->target->color, color, true);
}

void StrokeStyleWidget::check_color(const QColor& color)
{
    d->update_background(color);
    if ( d->can_update_target() )
        d->set(d->target->color, color, false);
    update();
    emit color_changed(color);
}

void StrokeStyleWidget::set_shape(model::Stroke* target)
{
    if ( d->target )
    {
        disconnect(d->target, &model::Object::property_changed, this, &StrokeStyleWidget::property_changed);
    }

    d->target = target;

    if ( target )
    {
        d->update_from_target();
        emit color_changed(d->target->color.get());
        connect(target, &model::Object::property_changed, this, &StrokeStyleWidget::property_changed);
        update();
    }
}

void StrokeStyleWidget::property_changed(const model::BaseProperty* prop)
{
    d->update_from_target();
    if ( prop == &d->target->color )
        emit color_changed(d->target->color.get());
    update();
}

void StrokeStyleWidget::check_miter(double w)
{
    if ( d->can_update_target() )
        d->set(d->target->miter_limit, w, false);
    update();
}

void StrokeStyleWidget::check_width(double w)
{
    if ( d->can_update_target() )
        d->set(d->target->width, w, false);
    update();
}

void StrokeStyleWidget::color_committed(const QColor& color)
{
    if ( d->can_update_target() )
        d->set(d->target->color, color, true);
}

void StrokeStyleWidget::commit_width()
{
    if ( d->can_update_target() && !qFuzzyCompare(d->target->width.get(), float(d->ui.spin_stroke_width->value())) )
        d->set(d->target->width, d->ui.spin_stroke_width->value(), true);
}
