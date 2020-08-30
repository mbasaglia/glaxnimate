#pragma once

#include <memory>
#include <QWidget>

#include "model/document.hpp"

class CompoundTimelineWidget : public QWidget
{
    Q_OBJECT
    
public:
    CompoundTimelineWidget(QWidget* parent = nullptr);
    ~CompoundTimelineWidget();
    
    void set_active(model::DocumentNode* node);
    void set_document(model::Document* document);
    void clear_document();
    
protected:
    void changeEvent ( QEvent* e ) override;
    
private slots:
    void select_property(const QModelIndex& index);
    void select_animatable(model::AnimatableBase* anim);
    void custom_context_menu(const QPoint& p);
    void add_keyframe();
    
private:
    class Private;
    std::unique_ptr<Private> d;
};
