#include "layer_view.hpp"

#include <QHeaderView>

#include <QtColorWidgets/ColorDelegate>

#include "item_models/document_node_model.hpp"
#include "item_models/comp_filter_model.hpp"


using namespace glaxnimate::gui;


class glaxnimate::gui::LayerView::Private
{
public:
    item_models::DocumentModelBase* base_model;
    item_models::CompFilterModel proxy_model;

    color_widgets::ColorDelegate color_delegate;
};

glaxnimate::gui::LayerView::LayerView(QWidget*parent)
    : CustomTreeView(parent), d(std::make_unique<Private>())
{
    header()->setStretchLastSection(false);
    setHeaderHidden(true);
    setDragDropMode(InternalMove);
    setDragEnabled(true);
}

glaxnimate::gui::LayerView::~LayerView() = default;

item_models::DocumentModelBase * glaxnimate::gui::LayerView::base_model() const
{
    return d->base_model;
}

void glaxnimate::gui::LayerView::set_base_model(item_models::DocumentModelBase* base_model)
{
    d->base_model = base_model;
    d->proxy_model.setSourceModel(base_model);


    setModel(&d->proxy_model);

    header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &d->color_delegate);
    header()->hideSection(item_models::DocumentNodeModel::ColumnUsers);

#ifdef Q_OS_ANDROID
    int icon_size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, nullptr);
    header()->setDefaultSectionSize(icon_size);
#endif

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LayerView::on_current_node_changed);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LayerView::on_selection_changed);
}



void glaxnimate::gui::LayerView::on_current_node_changed(const QModelIndex& index)
{
    emit current_node_changed(d->base_model->visual_node(d->proxy_model.mapToSource(index)));
}

void glaxnimate::gui::LayerView::on_selection_changed(const QItemSelection& selected, const QItemSelection& deselected)
{
    std::vector<model::VisualNode*> selected_nodes;
    std::vector<model::VisualNode*> deselected_nodes;

    for ( const auto& index : deselected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = d->base_model->visual_node(d->proxy_model.mapToSource(index)) )
                deselected_nodes.push_back(node);

    for ( const auto& index : selected.indexes() )
        if ( index.column() == 0 )
            if ( auto node = d->base_model->visual_node(d->proxy_model.mapToSource(index)) )
                selected_nodes.push_back(node);

    emit selection_changed(selected_nodes, deselected_nodes);
}

void glaxnimate::gui::LayerView::set_current_node(model::DocumentNode* node)
{
    if ( !node )
    {
        setCurrentIndex({});
    }
    else
    {
        auto index = d->proxy_model.mapFromSource(d->base_model->node_index(node));
        setCurrentIndex(index);
    }
}

glaxnimate::model::VisualNode* glaxnimate::gui::LayerView::node(const QModelIndex& index) const
{
    return d->base_model->visual_node(d->proxy_model.mapToSource(index));
}

void glaxnimate::gui::LayerView::set_composition(model::Composition* comp)
{
    d->proxy_model.set_composition(comp);
}

glaxnimate::model::VisualNode * glaxnimate::gui::LayerView::current_node() const
{
    return node(currentIndex());
}

void glaxnimate::gui::LayerView::replace_selection(model::VisualNode* node)
{
    auto index = d->proxy_model.mapFromSource(d->base_model->node_index(node));
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}


void glaxnimate::gui::LayerView::update_selection(const std::vector<model::VisualNode *>& selected, const std::vector<model::VisualNode *>& deselected)
{
    for ( model::VisualNode* node : deselected )
    {
        selectionModel()->select(
            d->proxy_model.mapFromSource(d->base_model->node_index(node)),
            QItemSelectionModel::Deselect|QItemSelectionModel::Rows
        );
    }

    for ( model::VisualNode* node : selected )
    {
        selectionModel()->select(
            d->proxy_model.mapFromSource(d->base_model->node_index(node)),
            QItemSelectionModel::Select|QItemSelectionModel::Rows
        );
    }
}


void glaxnimate::gui::LayerView::mouseReleaseEvent(QMouseEvent * event)
{
    CustomTreeView::mouseReleaseEvent(event);

    if ( event->button() == Qt::LeftButton )
    {
        auto index = indexAt(event->pos());
        auto node = this->node(index);
        if ( !node )
            return;

        if ( index.column() == item_models::DocumentNodeModel::ColumnVisible )
            node->visible.set(!node->visible.get());
        else if ( index.column() == item_models::DocumentNodeModel::ColumnLocked )
            node->locked.set(!node->locked.get());
    }
}
