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
    LayerView(QWidget* parent);
    ~LayerView();

    void set_models(item_models::DocumentModelBase* base_model, QAbstractProxyModel* proxy_model);

signals:
    void selection_changed(const std::vector<model::VisualNode*>& selected, const std::vector<model::VisualNode*>& deselected);
    void current_node_changed(model::VisualNode* node);

private:
    void on_selection_changed(const QItemSelection &selected, const QItemSelection &deselected);
    void on_current_node_changed(const QModelIndex& index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
