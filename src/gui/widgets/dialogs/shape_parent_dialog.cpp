#include "shape_parent_dialog.hpp"
#include "ui_shape_parent_dialog.h"
#include <QtColorWidgets/ColorDelegate>
#include "model/shapes/group.hpp"
#include "model/layers/shape_layer.hpp"

class ShapeParentDialog::Private
{
public:
    model::ShapeListProperty* current = nullptr;
    item_models::DocumentNodeModel* model = nullptr;
    Ui::ShapeParentDialog ui;
    color_widgets::ReadOnlyColorDelegate color_delegate;

    model::ShapeListProperty* get_popr(const QModelIndex& index)
    {
        auto node = model->node(index);
        if ( !node )
            return nullptr;
        if ( auto grp = qobject_cast<model::Group*>(node) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::ShapeLayer*>(node) )
            return &lay->shapes;
        return nullptr;
    }
};

ShapeParentDialog::ShapeParentDialog(item_models::DocumentNodeModel* model, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->model = model;
    d->ui.view_document_node->setModel(model);
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


