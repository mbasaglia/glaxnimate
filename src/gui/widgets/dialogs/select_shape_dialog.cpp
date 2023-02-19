#include "select_shape_dialog.hpp"
#include "ui_select_shape_dialog.h"
#include <QEvent>

#include <QtColorWidgets/ColorDelegate>

#include "item_models/node_type_proxy_model.hpp"

class glaxnimate::gui::SelectShapeDialog::Private
{
public:
    Private(item_models::DocumentNodeModel* model)
        : proxy(model)
    {
        proxy.allow<model::Shape>();
    }

    color_widgets::ReadOnlyColorDelegate color_delegate;
    item_models::NodeTypeProxyModel proxy;
    Ui::SelectShapeDialog ui;
    model::Shape* selected = nullptr;
};

glaxnimate::gui::SelectShapeDialog::SelectShapeDialog(item_models::DocumentNodeModel* model, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>(model))
{
    d->ui.setupUi(this);
    d->ui.view_document_node->setModel(&d->proxy);
    d->ui.view_document_node->expandAll();
    d->ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnName, QHeaderView::Stretch);
    d->ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnVisible);
    d->ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnLocked);
    d->ui.view_document_node->header()->hideSection(item_models::DocumentNodeModel::ColumnUsers);
    d->ui.view_document_node->header()->setSectionResizeMode(item_models::DocumentNodeModel::ColumnColor, QHeaderView::ResizeToContents);
    d->ui.view_document_node->setItemDelegateForColumn(item_models::DocumentNodeModel::ColumnColor, &d->color_delegate);
}

glaxnimate::gui::SelectShapeDialog::~SelectShapeDialog() = default;

void glaxnimate::gui::SelectShapeDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::SelectShapeDialog::select(const QModelIndex& index)
{
    auto node = d->proxy.node(index);
    if ( !node )
        d->selected = nullptr;
    else
        d->selected = node->cast<model::Shape>();
}


glaxnimate::model::Shape * glaxnimate::gui::SelectShapeDialog::shape() const
{
    return d->selected;
}

void glaxnimate::gui::SelectShapeDialog::set_shape(model::Shape* shape)
{
    if ( shape )
    {
        auto index = d->proxy.mapFromSource(
            d->proxy.source_model()->node_index(shape)
        );
        if ( index.isValid() )
        {
            d->selected = shape;
            d->ui.view_document_node->setCurrentIndex(index);
        }
    }
    else
    {
        d->selected = nullptr;
        d->ui.view_document_node->setCurrentIndex({});
    }
}
