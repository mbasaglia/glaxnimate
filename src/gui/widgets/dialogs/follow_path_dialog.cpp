#include "follow_path_dialog.hpp"
#include "ui_follow_path_dialog.h"
#include <QEvent>

#include "command/undo_macro_guard.hpp"
#include "command/animation_commands.hpp"
#include "item_models/node_type_proxy_model.hpp"
#include "widgets/dialogs/select_shape_dialog.hpp"

class glaxnimate::gui::FollowPathDialog::Private
{
public:
    enum Units
    {
        Frames,
        Seconds
    };

    Private(model::AnimatedProperty<QPointF>* property, item_models::DocumentNodeModel* model, FollowPathDialog* parent)
        : select_shape_dialog(model, parent), property(property)
    {
        ui.setupUi(parent);
        set_units(Frames);
        auto anim = property->object()->document()->main()->animation.get();
        ui.spin_start->setValue(anim->first_frame.get());
        QSignalBlocker be(ui.spin_end);
        QSignalBlocker bd(ui.spin_duration);
        ui.spin_end->setValue(anim->last_frame.get());
        ui.spin_duration->setValue(anim->last_frame.get() - anim->first_frame.get());
    }

    void set_frames(QDoubleSpinBox* box)
    {
        auto main = property->object()->document()->main();
        box->setSuffix(tr("f"));
        box->setDecimals(0);
        auto v = box->value();
        box->setMinimum(main->animation->first_frame.get());
        box->setMaximum(main->animation->last_frame.get());
        box->setValue(v * main->fps.get());
    }

    void set_seconds(QDoubleSpinBox* box)
    {
        auto main = property->object()->document()->main();
        auto fps = main->fps.get();
        box->setSuffix(tr("\""));
        box->setDecimals(2);
        auto v = box->value();
        box->setMinimum(main->animation->first_frame.get() / fps);
        box->setMaximum(main->animation->last_frame.get() / fps);
        box->setValue(v / fps);
    }

    void set_units(int index)
    {
        QSignalBlocker be(ui.spin_end);
        QSignalBlocker bd(ui.spin_duration);

        if ( index == Frames )
        {
            set_frames(ui.spin_start);
            set_frames(ui.spin_end);
            set_frames(ui.spin_duration);
        }
        else
        {
            set_seconds(ui.spin_start);
            set_seconds(ui.spin_end);
            set_seconds(ui.spin_duration);
        }
    }

    model::FrameTime frame(qreal time)
    {
        if ( ui.combo_units->currentIndex() == Frames )
            return time;
        return time * property->object()->document()->main()->fps.get();
    }

    Ui::FollowPathDialog ui;
    SelectShapeDialog select_shape_dialog;
    model::AnimatedProperty<QPointF>* property;
    model::Shape* shape = nullptr;
};

glaxnimate::gui::FollowPathDialog::FollowPathDialog(model::AnimatedProperty<QPointF>* property, item_models::DocumentNodeModel* model, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>(property, model, this))
{}

glaxnimate::gui::FollowPathDialog::~FollowPathDialog() = default;

void glaxnimate::gui::FollowPathDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::FollowPathDialog::apply()
{
    if ( !d->shape )
    {
        accept();
        return;
    }

    auto start = d->frame(d->ui.spin_start->value());
    auto end = d->frame(d->ui.spin_end->value());
    auto bezier = d->shape->to_bezier(d->shape->time());
    bezier.add_close_point();
    math::bezier::LengthData length(bezier, 20);

    if ( start == end || length.length() == 0 )
    {
        accept();
        return;
    }

    auto guard = command::UndoMacroGuard(tr("Follow Path"), d->property->object()->document());
    d->property->object()->push_command(new command::RemoveAllKeyframes(d->property, d->property->value()));

    for ( int i = 0; i < bezier.size(); i++ )
    {
        qreal point_length = i < bezier.size() - 1 ? length.child_start(i) : length.child_end(i-1);
        d->property->object()->push_command(new command::SetKeyframe(
            d->property,
            math::lerp(start, end, point_length / length.length()),
            QVariant::fromValue(bezier[i].pos),
            true,
            true
        ));
    }

    d->property->object()->push_command(new command::SetPositionBezier(
        d->property,
        bezier,
        true
    ));

    accept();
}

void glaxnimate::gui::FollowPathDialog::change_duration(double dur)
{
    QSignalBlocker be(d->ui.spin_end);
    d->ui.spin_end->setValue(d->ui.spin_start->value() + dur);
}

void glaxnimate::gui::FollowPathDialog::change_end(double end)
{
    QSignalBlocker bd(d->ui.spin_duration);
    d->ui.spin_duration->setValue(end - d->ui.spin_start->value());
}

void glaxnimate::gui::FollowPathDialog::change_units(int index)
{
    d->set_units(index);
}

void glaxnimate::gui::FollowPathDialog::select_path()
{
    d->select_shape_dialog.set_shape(d->shape);
    if ( d->select_shape_dialog.exec() == QDialog::Accepted )
    {
        d->shape = d->select_shape_dialog.shape();
        d->ui.line_shape_name->setText(d->shape ? d->shape->object_name() : "");
    }
}



