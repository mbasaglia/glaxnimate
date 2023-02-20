#pragma once

#include <QAbstractProxyModel>

#include "model/document_node.hpp"
#include "item_models/document_model_base.hpp"
#include "custom_treeview.hpp"


namespace glaxnimate::gui {

class LayerView : public CustomTreeView
{
    Q_OBJECT

public:
    LayerView(QWidget* parent = nullptr);
    ~LayerView();

    void set_base_model(item_models::DocumentModelBase* base_model);
    item_models::DocumentModelBase* base_model() const;
    void set_current_node(model::DocumentNode* node);
    model::VisualNode* node(const QModelIndex& index) const;
    model::VisualNode* current_node() const;

    void set_composition(model::Composition* comp);

    void replace_selection(model::VisualNode* node);
    void update_selection(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected);

signals:
    void selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected);
    void current_node_changed(model::VisualNode* node);

private:
    void on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void on_current_node_changed(const QModelIndex& index);

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
