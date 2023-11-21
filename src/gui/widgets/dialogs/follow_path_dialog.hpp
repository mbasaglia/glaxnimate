/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GLAXNIMATE_GUI_FOLLOWPATHDIALOG_H
#define GLAXNIMATE_GUI_FOLLOWPATHDIALOG_H

#include <memory>
#include <QDialog>

#include "model/animation/animatable.hpp"
#include "item_models/document_node_model.hpp"

namespace glaxnimate {
namespace gui {

class FollowPathDialog : public QDialog
{
    Q_OBJECT

public:
    FollowPathDialog(model::AnimatedProperty<QPointF>* property, model::Composition* comp, item_models::DocumentNodeModel* model, QWidget* parent = nullptr);
    ~FollowPathDialog();

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void apply();
    void change_units(int index);
    void change_duration(double dur);
    void change_end(double dur);
    void select_path();

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
}

#endif // GLAXNIMATE_GUI_FOLLOWPATHDIALOG_H
