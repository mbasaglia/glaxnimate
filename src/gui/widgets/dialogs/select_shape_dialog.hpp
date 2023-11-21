/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLAXNIMATE_GUI_SELECTSHAPEDIALOG_H
#define GLAXNIMATE_GUI_SELECTSHAPEDIALOG_H

#include <memory>
#include <QDialog>


#include "model/shapes/shape.hpp"
#include "item_models/document_node_model.hpp"

namespace glaxnimate {
namespace gui {

class SelectShapeDialog : public QDialog
{
    Q_OBJECT

public:
    SelectShapeDialog(item_models::DocumentNodeModel* model, QWidget* parent = nullptr);
    ~SelectShapeDialog();

    model::Shape* shape() const;
    void set_shape(model::Shape* shape);

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void select(const QModelIndex& index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
}

#endif // GLAXNIMATE_GUI_SELECTSHAPEDIALOG_H
