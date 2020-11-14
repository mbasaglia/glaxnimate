#include "keyframe_editor_widget.hpp"
#include "ui_keyframe_editor_widget.h"
#include "app/application.hpp"


namespace  {

void set_icon(QComboBox* box, int i, const char* ba)
{
    QString icon_name = QString("images/keyframe/%1/%2.svg");
    QString which;
    switch ( model::KeyframeTransition::Descriptive(i) )
    {
        case model::KeyframeTransition::Hold: which = "hold"; break;
        case model::KeyframeTransition::Linear: which = "linear"; break;
        case model::KeyframeTransition::Ease: which = "ease"; break;
        case model::KeyframeTransition::Custom: which = "custom"; break;
    }
    box->setItemIcon(i, QIcon(app::Application::instance()->data_file(icon_name.arg(ba).arg(which))));
}

void set_icons(QComboBox* box, const char* ba)
{
    for ( int i = 0; i < box->count(); i++ )
        set_icon(box, i, ba);
}

} // namespace

KeyframeEditorWidget::KeyframeEditorWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Ui::KeyframeEditorWidget>())
{
    d->setupUi(this);
    set_icons(d->combo_before, "start");
    set_icons(d->combo_after, "finish");

    connect(d->bezier_editor, &KeyframeTransitionWidget::before_changed, this, &KeyframeEditorWidget::update_before);
    connect(d->bezier_editor, &KeyframeTransitionWidget::after_changed, this, &KeyframeEditorWidget::update_after);
}

KeyframeEditorWidget::~KeyframeEditorWidget() = default;

void KeyframeEditorWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void KeyframeEditorWidget::set_target(model::KeyframeTransition* kft)
{
    d->bezier_editor->set_target(kft);
}

void KeyframeEditorWidget::preset_after(int index)
{
    if ( !d->bezier_editor->target() )
        return;

    d->bezier_editor->target()->set_after_descriptive(model::KeyframeTransition::Descriptive(index));
    d->bezier_editor->update();
}

void KeyframeEditorWidget::preset_before(int index)
{
    if ( !d->bezier_editor->target() )
        return;

    d->bezier_editor->target()->set_before_descriptive(model::KeyframeTransition::Descriptive(index));
    d->bezier_editor->update();
}

void KeyframeEditorWidget::update_after(model::KeyframeTransition::Descriptive v)
{
    d->combo_after->setCurrentIndex(int(v));
}

void KeyframeEditorWidget::update_before(model::KeyframeTransition::Descriptive v)
{
    d->combo_before->setCurrentIndex(int(v));
}
