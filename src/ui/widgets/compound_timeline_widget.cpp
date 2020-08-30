#include "compound_timeline_widget.hpp"
#include "ui_compound_timeline_widget.h"

#include <QMenu>

#include "model/item_models/property_model.hpp"
#include "ui/style/property_delegate.hpp"
#include "glaxnimate_app.hpp"
#include "command/animation_commands.hpp"

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
        ui.properties->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        ui.properties->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        ui.properties->horizontalHeader()->setFixedHeight(ui.timeline->header_height());
        
        ui.properties->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        ui.properties->verticalHeader()->setMinimumSectionSize(ui.timeline->row_height());
        ui.properties->verticalHeader()->setMaximumSectionSize(ui.timeline->row_height());
        ui.properties->verticalHeader()->setDefaultSectionSize(ui.timeline->row_height());
        
        connect(ui.timeline, &TimelineWidget::animatable_clicked, parent, &CompoundTimelineWidget::select_animatable);
        
        ui.action_add_keyframe->setIcon(
            QIcon(GlaxnimateApp::instance()->data_file("images/keyframe/status/key.svg"))
        );
        connect(ui.action_add_keyframe, &QAction::triggered, parent, &CompoundTimelineWidget::add_keyframe);
        action_title.setEnabled(false);
        menu_property.addAction(&action_title);
        menu_property.addAction(ui.action_add_keyframe);
        connect(parent, &QWidget::customContextMenuRequested, parent, &CompoundTimelineWidget::custom_context_menu);
        
    }
    
    Ui::CompoundTimelineWidget ui;
    model::PropertyModel property_model{true};
    PropertyDelegate property_delegate;
    QAction action_title;
    QMenu menu_property;
    model::AnimatableBase* menu_anim = nullptr;
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
        d->ui.retranslateUi(this);
    }
}

void CompoundTimelineWidget::set_active(model::DocumentNode* node)
{
    d->property_model.set_object(node);
    d->ui.timeline->set_active(node);
    d->menu_anim = nullptr;
}

void CompoundTimelineWidget::set_document(model::Document* document)
{
    d->property_model.set_document(document);
    d->ui.timeline->set_document(document);
    d->menu_anim = nullptr;
}

void CompoundTimelineWidget::clear_document()
{
    d->property_model.clear_document();
    d->ui.timeline->set_document(nullptr);
    d->menu_anim = nullptr;
}

void CompoundTimelineWidget::select_property(const QModelIndex& index)
{
    d->ui.timeline->select(d->property_model.animatable(index));
    d->ui.properties->viewport()->update();
}

void CompoundTimelineWidget::select_animatable(model::AnimatableBase* anim)
{
    d->ui.properties->setCurrentIndex(d->property_model.property_index(anim));
    d->ui.properties->viewport()->update();
}

void CompoundTimelineWidget::custom_context_menu(const QPoint& p)
{
    d->menu_anim = nullptr;
    QPoint glob = static_cast<QWidget*>(sender())->mapToGlobal(p);
    
    if ( d->ui.properties->rect().contains(p) )
    {
        d->menu_anim = d->property_model.animatable(
            d->ui.properties->indexAt(
                d->ui.properties->viewport()->mapFromGlobal(glob)
            )
        );
    }
    else
    {
        d->menu_anim = d->ui.timeline->animatable_at(
            d->ui.timeline->viewport()->mapFromGlobal(glob)
        );
    }
    
    if ( !d->menu_anim )
        return;
    
    d->action_title.setText(d->menu_anim->name());
    d->menu_property.exec(glob);
}

void CompoundTimelineWidget::add_keyframe()
{
    if ( !d->menu_anim )
        return;
    
    d->menu_anim->object()->document()->undo_stack().push(
        new command::SetKeyframe(d->menu_anim, d->menu_anim->time(), d->menu_anim->value(), true)
    );
}
