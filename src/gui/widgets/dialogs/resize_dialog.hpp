/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RESIZEDIALOG_H
#define RESIZEDIALOG_H

#include <QDialog>
#include <memory>

#include "model/assets/composition.hpp"

namespace glaxnimate::gui {

class ResizeDialog : public QDialog
{
    Q_OBJECT

public:
    ResizeDialog(QWidget* parent = nullptr);

    ~ResizeDialog();

    void resize_composition(model::Composition* comp);

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void width_changed(int w);
    void height_changed(int h);
    void lock_changed(bool locked);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // RESIZEDIALOG_H
