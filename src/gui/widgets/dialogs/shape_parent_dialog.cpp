#include "shape_parent_dialog.hpp"
#include "ui_shape_parent_dialog.h"
#include <QtColorWidgets/ColorDelegate>
#include "model/shapes/group.hpp"
#include "item_models/node_type_proxy_model.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class ShapeParentDialog::Private
{
public:
    model::ShapeListProperty* current = nullptr;
    item_models::DocumentNodeModel* model = nullptr;
    Ui::ShapeParentDialog ui;
    color_widgets::ReadOnlyColorDelegate color_delegate;
    item_models::NodeTypeProxyModel proxy;

    Private(item_models::DocumentNodeModel* model)
        : model(model), proxy(model)
    {
        proxy.allow<model::Group>();
        proxy.allow<model::Composition>();
    }

    model::ShapeListProperty* get_popr(const QModelIndex& index)
    {
        auto node = model->node(proxy.mapToSource(index));
        if ( !node )
            return nullptr;
        if ( auto grp = qobject_cast<model::Group*>(node) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::Composition*>(node) )
            return &lay->shapes;
        return nullptr;
    }
};

ShapeParentDialog::ShapeParentDialog(item_models::DocumentNodeModel* model, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>(model))
{
    d->ui.setupUi(this);
    d->ui.view_document_node->setModel(&d->proxy);
    d->ui.view_document_node->expandAll();
    d->ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    d->ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnVisible);
    d->ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnLocked);
    d->ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    d->ui.view_document_node->setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &d->color_delegate);
}

ShapeParentDialog::~ShapeParentDialog() = default;

void ShapeParentDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

model::ShapeListProperty * ShapeParentDialog::shape_parent() const
{
    return d->current;
}


void ShapeParentDialog::select(const QModelIndex& index)
{
    d->current = d->get_popr(index);
}

void ShapeParentDialog::select_and_accept(const QModelIndex& index)
{
    select(index);
    accept();
}

model::ShapeListProperty * ShapeParentDialog::get_shape_parent()
{
    if ( exec() == QDialog::Accepted )
        return d->current;
    return nullptr;
}


