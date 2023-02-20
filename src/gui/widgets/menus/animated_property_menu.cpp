#include "animated_property_menu.hpp"

#include <QClipboard>
#include <QMimeData>

#include "widgets/dialogs/follow_path_dialog.hpp"
#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"
#include "glaxnimate_app.hpp"
#include "widgets/dialogs/glaxnimate_window.hpp"

using namespace glaxnimate::gui;

class glaxnimate::gui::AnimatedPropertyMenu::Private
{
public:
    Private(AnimatedPropertyMenu* parent)
    {
        parent->setIcon(QIcon::fromTheme("label"));
        action_title = parent->addSeparator();

        parent->addAction(&action_kf_paste);
        action_kf_paste.setIcon(QIcon::fromTheme("edit-paste"));
        connect(&action_kf_paste, &QAction::triggered, parent, &AnimatedPropertyMenu::paste_keyframe);

        parent->addAction(&action_add_keyframe);
        action_add_keyframe.setIcon(
            QIcon(GlaxnimateApp::instance()->data_file("images/icons/keyframe-add.svg"))
        );
        connect(&action_add_keyframe, &QAction::triggered, parent, &AnimatedPropertyMenu::add_keyframe);

        parent->addAction(&action_remove_keyframe);
        action_remove_keyframe.setIcon(
            QIcon(GlaxnimateApp::instance()->data_file("images/icons/keyframe-remove.svg"))
        );
        connect(&action_remove_keyframe, &QAction::triggered, parent, &AnimatedPropertyMenu::remove_keyframe);

        action_remove_all_keyframes.setIcon(
            QIcon(GlaxnimateApp::instance()->data_file("images/icons/keyframe-remove.svg"))
        );
        connect(&action_remove_all_keyframes, &QAction::triggered, parent, &AnimatedPropertyMenu::remove_all_keyframes);
        parent->addAction(&action_remove_all_keyframes);

        parent->addSeparator();

        action_kf_loop.setIcon(QIcon::fromTheme("media-repeat-all"));
        connect(&action_kf_loop, &QAction::triggered, parent, &AnimatedPropertyMenu::loop_keyframes);
        parent->addAction(&action_kf_loop);

        action_follow_path.setIcon(QIcon::fromTheme("draw-bezier-curves"));
        connect(&action_follow_path, &QAction::triggered, parent, &AnimatedPropertyMenu::follow_path);
        parent->addAction(&action_follow_path);
        action_follow_path.setVisible(false);

        retranslate_menu();
    }

    void retranslate_menu()
    {
        action_remove_all_keyframes.setText(tr("Clear Animations"));
        action_kf_paste.setText(tr("Paste Keyframe"));
        action_kf_loop.setText(tr("Loop Animation"));
        action_follow_path.setText(tr("Follow Path..."));
        action_add_keyframe.setText(tr("Add Keyframe"));
        action_remove_keyframe.setText(tr("Remove Keyframe"));
    }

    model::AnimatableBase* property = nullptr;
    QAction* action_title;
    QAction action_kf_loop;
    QAction action_kf_paste;
    QAction action_follow_path;
    QAction action_add_keyframe;
    QAction action_remove_keyframe;
    QAction action_remove_all_keyframes;
    SelectionManager* window = nullptr;
};

glaxnimate::gui::AnimatedPropertyMenu::AnimatedPropertyMenu(QWidget* parent)
    : QMenu(parent), d(std::make_unique<Private>(this))
{
}

glaxnimate::gui::AnimatedPropertyMenu::~AnimatedPropertyMenu() = default;

void glaxnimate::gui::AnimatedPropertyMenu::paste_keyframe()
{
    if ( !d->property )
        return;

    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    if ( !data->hasFormat("application/x.glaxnimate-keyframe") )
        return;

    QByteArray encoded = data->data("application/x.glaxnimate-keyframe");
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int type = model::PropertyTraits::Unknown;
    stream >> type;
    if ( type != d->property->traits().type )
        return;

    QVariant value;
    stream >> value;

    d->property->object()->push_command(
        new command::SetKeyframe(d->property, d->property->time(), value, true)
    );
}

void glaxnimate::gui::AnimatedPropertyMenu::loop_keyframes()
{
    if ( !d->property || d->property->keyframe_count() < 1 )
        return;

    d->property->object()->push_command(new command::SetKeyframe(
        d->property,
        d->property->object()->document()->main()->animation->last_frame.get(),
        d->property->keyframe(0)->value(),
        true
    ));
}

void glaxnimate::gui::AnimatedPropertyMenu::follow_path()
{
    if ( d->property && d->property->traits().type == model::PropertyTraits::Point )
    {
        auto prop = static_cast<model::AnimatedProperty<QPointF>*>(d->property);
        FollowPathDialog(prop, d->window->model(), parentWidget()).exec();
    }
}

void glaxnimate::gui::AnimatedPropertyMenu::remove_all_keyframes()
{
    if ( !d->property )
        return;

    d->property->clear_keyframes_undoable();
}

void glaxnimate::gui::AnimatedPropertyMenu::set_controller(glaxnimate::gui::SelectionManager* window)
{
    d->window = window;
}

void glaxnimate::gui::AnimatedPropertyMenu::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslate_menu();
    }
}

void glaxnimate::gui::AnimatedPropertyMenu::set_property(model::AnimatableBase* property)
{
    d->property = property;
    if ( property )
    {
        setTitle(d->property->name());
        d->action_title->setText(d->property->name());
        d->action_follow_path.setVisible(property->traits().type == model::PropertyTraits::Point);
        refresh_actions();
    }
}

void glaxnimate::gui::AnimatedPropertyMenu::refresh_actions()
{
    if ( d->property )
    {
        bool has_kf = d->property->has_keyframe(d->property->time());
        d->action_add_keyframe.setEnabled(!has_kf);
        d->action_remove_keyframe.setEnabled(has_kf);
        d->action_remove_all_keyframes.setEnabled(d->property->keyframe_count() > 0);
        d->action_kf_loop.setEnabled(d->property->keyframe_count() > 0);
        d->action_kf_paste.setEnabled(can_paste());
    }
}


glaxnimate::model::AnimatableBase * glaxnimate::gui::AnimatedPropertyMenu::property() const
{
    return d->property;
}

bool glaxnimate::gui::AnimatedPropertyMenu::can_paste() const
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    if ( d->property && data->hasFormat("application/x.glaxnimate-keyframe") )
    {
        QByteArray encoded = data->data("application/x.glaxnimate-keyframe");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        int type = model::PropertyTraits::Unknown;
        stream >> type;
        return type == d->property->traits().type;
    }

    return false;
}


void glaxnimate::gui::AnimatedPropertyMenu::add_keyframe()
{
    if ( d->property )
        d->property->add_smooth_keyframe_undoable(d->property->time(), d->property->value());
}

void glaxnimate::gui::AnimatedPropertyMenu::remove_keyframe()
{
    if ( d->property )
        d->property->object()->push_command(
            new command::RemoveKeyframeTime(d->property, d->property->time())
        );
}

