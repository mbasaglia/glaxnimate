#pragma once

#include <memory>
#include <QWidget>

#include "model/document.hpp"

class GlaxnimateWindow;
class QAbstractItemModel;
class TimelineWidget;
class QItemSelection;
class CompoundTimelineWidget : public QWidget
{
    Q_OBJECT
    
public:
    CompoundTimelineWidget(QWidget* parent = nullptr);
    ~CompoundTimelineWidget();
    
    void set_current_node(model::DocumentNode* node);
    void set_document(model::Document* document);
    void set_composition(model::Composition* comp);
    void clear_document();
    QByteArray save_state() const;
    void load_state(const QByteArray& state);
    void set_controller(GlaxnimateWindow* window);
    QAbstractItemModel* filtered_model() const;
    QAbstractItemModel* raw_model() const;
    TimelineWidget* timeline() const;

    void reset_view();

    void select(const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected);

signals:
    void switch_composition(model::Composition* comp, int index);
    void current_node_changed(model::VisualNode* node);
    void selection_changed(const std::vector<model::VisualNode*>& selected,  const std::vector<model::VisualNode*>& deselected);
    
protected:
    void changeEvent ( QEvent* e ) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
private slots:
    void select_index(const QModelIndex& index);
    void _on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void select_line(quintptr id, bool selected, bool replace_selection);
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
    void rows_removed( const QModelIndex& index, int first, int last );
    
private:
    class Private;
    std::unique_ptr<Private> d;
};
