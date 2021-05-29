#pragma once

#include <memory>
#include <QWidget>

#include "model/document.hpp"

class GlaxnimateWindow;
class QAbstractItemModel;
class TimelineWidget;
class CompoundTimelineWidget : public QWidget
{
    Q_OBJECT
    
public:
    CompoundTimelineWidget(QWidget* parent = nullptr);
    ~CompoundTimelineWidget();
    
    void set_active(model::DocumentNode* node);
    void set_document(model::Document* document);
    void set_composition(model::Composition* comp);
    void clear_document();
    QByteArray save_state() const;
    void load_state(const QByteArray& state);
    void set_controller(GlaxnimateWindow* window);
    QAbstractItemModel* model() const;
    TimelineWidget* timeline() const;

signals:
    void switch_composition(model::Composition* comp, int index);
    
protected:
    void changeEvent ( QEvent* e ) override;
    
private slots:
    void select_index(const QModelIndex& index);
    void select_line(quintptr id);
    void custom_context_menu(const QPoint& p);
    void add_keyframe();
    void remove_keyframe();
    void on_scroll(int amount);
    void keyframe_action_enter();
    void keyframe_action_exit();
    void copy_keyframe();
    void paste_keyframe();
    void collapse_index(const QModelIndex& index);
    void expand_index(const QModelIndex& index);
    void click_index ( const QModelIndex& index );
    
private:
    class Private;
    std::unique_ptr<Private> d;
};
