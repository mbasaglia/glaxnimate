/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <memory>
#include <QDialog>

#include "model/document.hpp"

namespace glaxnimate::gui {

class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    StartupDialog(QWidget* parent = nullptr);
    ~StartupDialog();

    std::unique_ptr<model::Document> create() const;

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void reload_presets();
    void select_preset(const QModelIndex& index);
    void click_recent(const QModelIndex& index);
    void update_time_units();
    void update_startup_enabled(bool checked);

Q_SIGNALS:
    void open_recent(const QString& path);
    void open_browse();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // STARTUPDIALOG_H
