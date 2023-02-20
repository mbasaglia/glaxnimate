/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WIDGETPALETTEEDITOR_H
#define WIDGETPALETTEEDITOR_H

#include <memory>
#include <QWidget>
#include "app/settings/palette_settings.hpp"

class WidgetPaletteEditor : public QWidget
{
    Q_OBJECT

public:
    WidgetPaletteEditor ( app::settings::PaletteSettings* settings, QWidget* parent = nullptr );
    ~WidgetPaletteEditor();


private slots:
    void add_palette();
    void remove_palette();
    void update_color(int row, int column);
    void select_palette(const QString& name);

    void apply_palette();

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // WIDGETPALETTEEDITOR_H
