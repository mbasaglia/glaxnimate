#include "layer_view.hpp"

#include <QHeaderView>

#include <QtColorWidgets/ColorDelegate>

#include "item_models/document_node_model.hpp"

class glaxnimate::gui::LayerView::Private
{
public:
    item_models::DocumentModelBase* base_model = nullptr;
    QAbstractProxyModel* proxy_model = nullptr;

    color_widgets::ColorDelegate color_delegate;
};

glaxnimate::gui::LayerView::LayerView(QWidget*parent)
    : CustomTreeView(parent), d(std::make_unique<Private>())
{
}

glaxnimate::gui::LayerView::~LayerView() = default;

void glaxnimate::gui::LayerView::set_models(item_models::DocumentModelBase* base_model, QAbstractProxyModel* proxy_model)
{
    d->base_model = base_model;
    d->proxy_model = proxy_model;
    setModel(proxy_model);

    header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &d->color_delegate);
    header()->hideSection(item_models::DocumentNodeModel::ColumnUsers);

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LayerView::on_current_node_changed);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LayerView::on_selection_changed);
}

void glaxnimate::gui::LayerView::on_current_node_changed(const QModelIndex& index)
{
    emit current_node_changed(d->base_model->visual_node(d->proxy_model->mapToSource(index)));
}

void glaxnimate::gui::LayerView::on_selection_changed(const QItemSelection& selected, const QItemSelection& deselected)
{
    std::vector<model::VisualNode*> selected_nodes;
    std::vector<model::VisualNode*> deselected_nodes;

    for ( const auto& index : deselected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = d->base_model->visual_node(d->proxy_model->mapToSource(index)) )
                deselected_nodes.push_back(node);

    for ( const auto& index : selected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = d->base_model->visual_node(d->proxy_model->mapToSource(index)) )
                selected_nodes.push_back(node);

    emit selection_changed(selected_nodes, deselected_nodes);
}

