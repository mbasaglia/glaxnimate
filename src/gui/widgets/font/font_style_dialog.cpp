/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "font_style_dialog.hpp"
#include "ui_font_style_dialog.h"

#include <QEvent>
#include "app/widgets/no_close_on_enter.hpp"

class glaxnimate::gui::font::FontStyleDialog::Private
{
public:
    Ui::FontStyleDialog ui;
    app::widgets::NoCloseOnEnter ncoe;

    void set_preview_font()
    {
        QFont font;

        if ( ui.tab_widget->currentWidget() == ui.tab_google )
            font = ui.google_fonts_widget->selected_font();
        else if ( ui.tab_widget->currentWidget() == ui.tab_custom )
            font = ui.custom_font_widget->selected_font();
        else
            font = ui.widget_system->selected_font();

        ui.preview->set_font(font);
    }
};


glaxnimate::gui::font::FontStyleDialog::FontStyleDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    installEventFilter(&d->ncoe);
    connect(d->ui.tab_widget, &QTabWidget::currentChanged, this, [this]{
        d->set_preview_font();
    });
}

glaxnimate::gui::font::FontStyleDialog::~FontStyleDialog() = default;

void glaxnimate::gui::font::FontStyleDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

QFont glaxnimate::gui::font::FontStyleDialog::selected_font() const
{
    return d->ui.preview->selected_font();
}

void glaxnimate::gui::font::FontStyleDialog::set_font(const QFont& font)
{
    d->ui.widget_system->set_font(font);
    d->ui.google_fonts_widget->set_font_size(font.pointSizeF());
    d->ui.custom_font_widget->set_font_size(font.pointSizeF());
}

void glaxnimate::gui::font::FontStyleDialog::set_preview_text(const QString& text)
{
    d->ui.preview->set_text(text);
}

QStringList glaxnimate::gui::font::FontStyleDialog::favourites() const
{
    return d->ui.widget_system->model().favourites();
}

void glaxnimate::gui::font::FontStyleDialog::set_favourites(const QStringList& fav)
{
    d->ui.widget_system->model().set_favourites(fav);
}

void glaxnimate::gui::font::FontStyleDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    bool google_enabled = d->ui.google_fonts_widget->model().has_token();
    d->ui.tab_google->setEnabled(google_enabled);
    if ( !google_enabled && d->ui.tab_widget->currentWidget() == d->ui.tab_google )
        d->ui.tab_widget->setCurrentIndex(0);
    d->set_preview_font();
}

glaxnimate::model::CustomFont glaxnimate::gui::font::FontStyleDialog::custom_font() const
{
    if ( d->ui.tab_widget->currentWidget() == d->ui.tab_google )
        return d->ui.google_fonts_widget->custom_font();
    else if ( d->ui.tab_widget->currentWidget() == d->ui.tab_custom )
        return d->ui.custom_font_widget->custom_font();

    return {};
}
