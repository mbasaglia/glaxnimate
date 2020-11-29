#include "compound_timeline_widget.hpp"
#include "ui_compound_timeline_widget.h"

#include <QMenu>
#include <QScrollBar>
#include <QClipboard>
#include <QMimeData>

#include "model/shapes/precomp_layer.hpp"
#include "command/animation_commands.hpp"
#include "command/undo_macro_guard.hpp"

#include "item_models/property_model.hpp"
#include "glaxnimate_app.hpp"

#include "style/property_delegate.hpp"
#include "widgets/dialogs/keyframe_editor_dialog.hpp"
#include "widgets/node_menu.hpp"

class CompoundTimelineWidget::Private
{
public:
    void setupUi(CompoundTimelineWidget* parent)
    {
        ui.setupUi(parent);

        QPalette prop_pal = ui.properties->palette();
        prop_pal.setBrush(
            QPalette::Inactive,
            QPalette::Highlight,
            prop_pal.brush(QPalette::Active, QPalette::Highlight)
        );
        prop_pal.setBrush(
            QPalette::Inactive,
            QPalette::HighlightedText,
            prop_pal.brush(QPalette::Active, QPalette::HighlightedText)
        );
        ui.properties->setPalette(prop_pal);

        ui.properties->setModel(&property_model);
        connect(&property_model, &QAbstractItemModel::dataChanged,
                ui.properties->viewport(), (void (QWidget::*)())&QWidget::update);
        ui.properties->setItemDelegateForColumn(1, &property_delegate);
        ui.properties->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui.properties->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui.properties->header()->setFixedHeight(ui.timeline->header_height());
        property_delegate.set_forced_height(ui.timeline->header_height());


        ui.properties->verticalScrollBar()->setPageStep(ui.scrollbar->pageStep());
        connect(ui.properties->verticalScrollBar(), &QScrollBar::valueChanged, ui.scrollbar, &QScrollBar::setValue);
        connect(ui.scrollbar, &QScrollBar::valueChanged, ui.properties->verticalScrollBar(), &QScrollBar::setValue);
        connect(ui.properties->verticalScrollBar(), &QScrollBar::rangeChanged, ui.scrollbar, &QScrollBar::setRange);
        connect(ui.scrollbar, &QScrollBar::valueChanged, parent, &CompoundTimelineWidget::on_scroll);

        connect(ui.timeline, &TimelineWidget::property_clicked, parent, &CompoundTimelineWidget::select_property);
        connect(ui.timeline, &TimelineWidget::object_clicked, parent, &CompoundTimelineWidget::select_object);

        ui.action_add_keyframe->setIcon(
            QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/status/key.svg"))
        );
        connect(ui.action_add_keyframe, &QAction::triggered, parent, &CompoundTimelineWidget::add_keyframe);
        action_title = menu_property.addSeparator();
        menu_property.addAction(ui.action_add_keyframe);

        menu_property.addAction(&action_kf_paste1);
        action_kf_paste1.setIcon(QIcon::fromTheme("edit-paste"));
        connect(&action_kf_paste1, &QAction::triggered, parent, &CompoundTimelineWidget::paste_keyframe);

        connect(parent, &QWidget::customContextMenuRequested, parent, &CompoundTimelineWidget::custom_context_menu);

        setup_menu(parent);
    }

    void setup_menu(CompoundTimelineWidget* parent)
    {
        action_enter_hold.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/hold.svg")));
        action_enter_linear.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/linear.svg")));
        action_enter_ease.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/ease.svg")));
        action_enter_custom.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/custom.svg")));
        action_exit_hold.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/hold.svg")));
        action_exit_linear.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/linear.svg")));
        action_exit_ease.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/ease.svg")));
        action_exit_custom.setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/custom.svg")));

        action_enter_hold.setActionGroup(&enter);
        action_enter_linear.setActionGroup(&enter);
        action_enter_ease.setActionGroup(&enter);
        action_enter_custom.setActionGroup(&enter);
        action_exit_hold.setActionGroup(&exit);
        action_exit_linear.setActionGroup(&exit);
        action_exit_ease.setActionGroup(&exit);
        action_exit_custom.setActionGroup(&exit);

        action_enter_hold.setData(QVariant::fromValue(model::KeyframeTransition::Hold));
        action_enter_linear.setData(QVariant::fromValue(model::KeyframeTransition::Linear));
        action_enter_ease.setData(QVariant::fromValue(model::KeyframeTransition::Ease));
        action_exit_hold.setData(QVariant::fromValue(model::KeyframeTransition::Hold));
        action_exit_linear.setData(QVariant::fromValue(model::KeyframeTransition::Linear));
        action_exit_ease.setData(QVariant::fromValue(model::KeyframeTransition::Ease));

        menu_keyframe.addAction(&action_kf_remove);
        action_kf_remove.setIcon(QIcon::fromTheme("edit-delete-remove"));
        connect(&action_kf_remove, &QAction::triggered, parent, &CompoundTimelineWidget::remove_keyframe);

        menu_keyframe.addAction(&action_kf_copy);
        action_kf_copy.setIcon(QIcon::fromTheme("edit-copy"));
        connect(&action_kf_copy, &QAction::triggered, parent, &CompoundTimelineWidget::copy_keyframe);

        menu_keyframe.addAction(&action_kf_paste2);
        action_kf_paste2.setIcon(QIcon::fromTheme("edit-paste"));
        connect(&action_kf_paste2, &QAction::triggered, parent, &CompoundTimelineWidget::paste_keyframe);

        action_enter = menu_keyframe.addSeparator();
        for ( QAction* ac : enter.actions() )
        {
            ac->setCheckable(true);
            menu_keyframe.addAction(ac);
            connect(ac, &QAction::triggered, parent, &CompoundTimelineWidget::keyframe_action_enter);
        }

        action_exit = menu_keyframe.addSeparator();
        for ( QAction* ac : exit.actions() )
        {
            ac->setCheckable(true);
            menu_keyframe.addAction(ac);
            connect(ac, &QAction::triggered, parent, &CompoundTimelineWidget::keyframe_action_exit);
        }

        retranslate_menu();
    }

    void retranslate_menu()
    {
        action_enter->setText(tr("Transition From Previous"));
        action_exit->setText(tr("Transition To Next"));

        action_enter_hold.setText(tr("Hold"));
        action_exit_hold.setText(action_enter_hold.text());

        action_enter_linear.setText(tr("Linear"));
        action_exit_linear.setText(action_enter_linear.text());

        action_enter_ease.setText(tr("Ease"));
        action_exit_ease.setText(action_enter_ease.text());

        action_enter_custom.setText(tr("Custom..."));
        action_exit_custom.setText(action_enter_custom.text());

        action_kf_remove.setText(tr("Remove Keyframe"));

        action_kf_copy.setText(tr("Copy Keyframe"));
        action_kf_paste1.setText(tr("Paste Keyframe"));
        action_kf_paste2.setText(action_kf_paste1.text());
    }

    void retranslateUi(CompoundTimelineWidget* parent)
    {
        ui.retranslateUi(parent);
        retranslate_menu();
    }

    void clear_menu_data()
    {
        menu_anim = nullptr;
        menu_kf_enter = nullptr;
        menu_kf_exit = nullptr;
    }

    void keyframe_action(CompoundTimelineWidget* parent, model::KeyframeBase* keyframe, bool before_transition, QAction* action)
    {

        if ( !keyframe || !menu_anim )
            return;

        int index = menu_anim->keyframe_index(keyframe);
        if ( index == -1 )
            return;

        QVariant data = action->data();
        if ( data.isValid() )
        {
            menu_anim->object()->push_command(
                new command::SetKeyframeTransition(
                    menu_anim, index, data.value<model::KeyframeTransition::Descriptive>(),
                    before_transition ? keyframe->transition().before() : keyframe->transition().after(),
                    before_transition
                )
            );
        }
        else
        {
            KeyframeEditorDialog keyframe_editor(keyframe->transition(), parent);
            keyframe_editor.setWindowModality(Qt::ApplicationModal);
            if ( keyframe_editor.exec() )
            {
                menu_anim->object()->push_command(
                    new command::SetKeyframeTransition(
                        menu_anim, index,
                        keyframe_editor.transition()
                    )
                );
            }
        }
    }

    void toggle_paste()
    {
        const QMimeData* data = QGuiApplication::clipboard()->mimeData();
        bool enabled = false;
        if ( menu_anim && data->hasFormat("application/x.glaxnimate-keyframe") )
        {
            QByteArray encoded = data->data("application/x.glaxnimate-keyframe");
            QDataStream stream(&encoded, QIODevice::ReadOnly);
            int type = model::PropertyTraits::Unknown;
            stream >> type;
            enabled = type == menu_anim->traits().type;
        }

        action_kf_paste1.setEnabled(enabled);
        action_kf_paste2.setEnabled(enabled);
    }



    Ui::CompoundTimelineWidget ui;
    item_models::PropertyModel property_model{true};
    style::PropertyDelegate property_delegate;

    QAction* action_title;
    QMenu menu_property;

    QAction* action_enter;
    QAction* action_exit;
    QAction action_kf_remove;
    QAction action_enter_hold;
    QAction action_enter_linear;
    QAction action_enter_ease;
    QAction action_enter_custom;
    QAction action_exit_hold;
    QAction action_exit_linear;
    QAction action_exit_ease;
    QAction action_exit_custom;
    QAction action_kf_copy;
    QAction action_kf_paste1;
    QAction action_kf_paste2;
    QMenu menu_keyframe;
    QActionGroup enter{&menu_keyframe};
    QActionGroup exit{&menu_keyframe};

    model::AnimatableBase* menu_anim = nullptr;
    model::KeyframeBase* menu_kf_enter = nullptr;
    model::KeyframeBase* menu_kf_exit = nullptr;

    GlaxnimateWindow* window = nullptr;

};

CompoundTimelineWidget::CompoundTimelineWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->setupUi(this);
}


CompoundTimelineWidget::~CompoundTimelineWidget() = default;

void CompoundTimelineWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void CompoundTimelineWidget::set_active(model::DocumentNode* node)
{
    d->property_model.set_object(node);
    d->ui.timeline->set_active(node);
    if ( node )
    {
        auto mo = node->metaObject();
        if ( mo->inherits(&model::Layer::staticMetaObject) || mo->inherits(&model::Composition::staticMetaObject) )
        {
            for ( int i = node->docnode_child_count() - 1; i >= 0; i-- )
            {
                auto child = node->docnode_child(i);
                auto ch_mo = child->metaObject();
                if ( ch_mo->inherits(&model::Layer::staticMetaObject) ||
                    ch_mo->inherits(&model::PreCompLayer::staticMetaObject) )
                {
                    d->ui.timeline->add_object_without_properties(child);
                    d->property_model.add_object_without_properties(child);
                }
            }
        }
        else if ( mo->inherits(&model::Group::staticMetaObject) )
        {
            for ( int i = node->docnode_child_count() - 1; i >= 0; i-- )
            {
                auto child = node->docnode_child(i);
                auto ch_mo = child->metaObject();
                if ( ch_mo->inherits(&model::Layer::staticMetaObject) ||
                    ch_mo->inherits(&model::PreCompLayer::staticMetaObject)
                )
                {
                    d->ui.timeline->add_object_without_properties(child);
                    d->property_model.add_object_without_properties(child);
                }
                else if ( !ch_mo->inherits(&model::Group::staticMetaObject) )
                {
                    d->ui.timeline->add_object(child);
                    d->property_model.add_object(child);
                }
            }
        }
    }
    d->ui.properties->expandAll();
    d->clear_menu_data();
}

void CompoundTimelineWidget::set_document(model::Document* document)
{
    d->property_model.set_document(document);
    d->ui.timeline->set_document(document);
    d->clear_menu_data();
}

void CompoundTimelineWidget::clear_document()
{
    d->property_model.clear_document();
    d->ui.timeline->set_document(nullptr);
    d->clear_menu_data();
}

void CompoundTimelineWidget::select_index(const QModelIndex& index)
{
    d->ui.timeline->select(d->property_model.item(index));
    d->ui.properties->viewport()->update();
}

void CompoundTimelineWidget::select_property(model::BaseProperty* anim)
{
    d->ui.properties->setCurrentIndex(d->property_model.property_index(anim));
    d->ui.properties->viewport()->update();
}

void CompoundTimelineWidget::select_object(model::Object* anim)
{
    d->ui.properties->setCurrentIndex(d->property_model.object_index(anim));
    d->ui.properties->viewport()->update();
}


void CompoundTimelineWidget::custom_context_menu(const QPoint& p)
{
    d->clear_menu_data();

    QPoint glob = static_cast<QWidget*>(sender())->mapToGlobal(p);

    item_models::PropertyModel::Item item;
    if ( d->ui.properties->rect().contains(p) )
    {
        item = d->property_model.item(
            d->ui.properties->indexAt(
                d->ui.properties->viewport()->mapFromGlobal(glob)
            )
        );
    }
    else
    {
        std::tie(d->menu_kf_enter, d->menu_kf_exit) = d->ui.timeline->keyframe_at(
            d->ui.timeline->viewport()->mapFromGlobal(glob)
        );

        item = d->ui.timeline->item_at(
            d->ui.timeline->viewport()->mapFromGlobal(glob)
        );
    }

    d->menu_anim = item.animatable;

    if ( d->menu_kf_exit )
    {
        d->enter.setEnabled(d->menu_kf_enter);
        if ( d->menu_kf_enter )
        {
            switch ( d->menu_kf_enter->transition().after_descriptive() )
            {
                case model::KeyframeTransition::Hold:
                    d->action_enter_hold.setChecked(true);
                    break;
                case model::KeyframeTransition::Linear:
                    d->action_enter_linear.setChecked(true);
                    break;
                case model::KeyframeTransition::Ease:
                    d->action_enter_ease.setChecked(true);
                    break;
                default:
                    d->action_enter_custom.setChecked(true);
                    break;
            }
        }

        switch ( d->menu_kf_exit->transition().before_descriptive() )
        {
            case model::KeyframeTransition::Hold:
                d->action_exit_hold.setChecked(true);
                break;
            case model::KeyframeTransition::Linear:
                d->action_exit_linear.setChecked(true);
                break;
            case model::KeyframeTransition::Ease:
                d->action_exit_ease.setChecked(true);
                break;
            default:
                d->action_exit_custom.setChecked(true);
                break;
        }

        d->toggle_paste();
        d->menu_keyframe.exec(glob);
    }
    else if ( d->menu_anim )
    {
        d->action_title->setText(d->menu_anim->name());
        d->toggle_paste();
        d->menu_property.exec(glob);
    }
    else if ( auto dn = qobject_cast<model::DocumentNode*>(item.object) )
    {
        NodeMenu(dn, d->window, this).exec(glob);
    }
}

void CompoundTimelineWidget::add_keyframe()
{
    if ( !d->menu_anim )
        return;

    d->menu_anim->object()->push_command(
        new command::SetKeyframe(d->menu_anim, d->ui.timeline->highlighted_time(), d->menu_anim->value(), true)
    );
}

void CompoundTimelineWidget::on_scroll(int amount)
{
    int scroll = -d->ui.timeline->header_height() + amount * d->ui.timeline->row_height();
    d->ui.timeline->verticalScrollBar()->setValue(scroll);
}

void CompoundTimelineWidget::keyframe_action_enter()
{
    d->keyframe_action(this, d->menu_kf_enter, false, static_cast<QAction*>(sender()));
}

void CompoundTimelineWidget::keyframe_action_exit()
{
    d->keyframe_action(this, d->menu_kf_exit, true, static_cast<QAction*>(sender()));
}

void CompoundTimelineWidget::remove_keyframe()
{
    if ( !d->menu_kf_exit || !d->menu_anim )
        return;

    d->menu_anim->object()->document()->undo_stack().push(
        new command::RemoveKeyframeTime(d->menu_anim, d->menu_kf_exit->time())
    );
}

void CompoundTimelineWidget::load_state(const QByteArray& state)
{
    d->ui.splitter->restoreState(state);
}

QByteArray CompoundTimelineWidget::save_state() const
{
    return d->ui.splitter->saveState();
}

void CompoundTimelineWidget::set_controller(GlaxnimateWindow* window)
{
    d->window = window;
}

void CompoundTimelineWidget::copy_keyframe()
{
    if ( !d->menu_kf_exit || !d->menu_anim )
        return;

    QMimeData* data = new QMimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << int(d->menu_anim->traits().type);
    stream << d->menu_kf_exit->value();
    /// \todo tangents for position keyframes
    data->setData("application/x.glaxnimate-keyframe", encoded);
    QGuiApplication::clipboard()->setMimeData(data);
}

void CompoundTimelineWidget::paste_keyframe()
{
    if ( !d->menu_anim )
        return;

    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    if ( !data->hasFormat("application/x.glaxnimate-keyframe") )
        return;

    QByteArray encoded = data->data("application/x.glaxnimate-keyframe");
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int type = model::PropertyTraits::Unknown;
    stream >> type;
    if ( type != d->menu_anim->traits().type )
        return;

    QVariant value;
    stream >> value;
    /// \todo tangents for position keyframes

    d->menu_anim->object()->push_command(
        new command::SetKeyframe(d->menu_anim, d->ui.timeline->highlighted_time(), value, true)
    );
}
