/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "external_font_widget.hpp"
#include "ui_external_font_widget.h"

#include <QEvent>
#include <QStandardItemModel>
#include <QFileDialog>

#include "widgets/font/font_loader.hpp"
#include "font_delegate.hpp"

class glaxnimate::gui::font::ExternalFontWidget::Private
{
public:
    Ui::ExternalFontWidget ui;
    FontLoader loader;
    std::map<QString, std::vector<model::CustomFont>> fonts;
    QStandardItemModel font_model;
    QUrl url;
    ExternalFontWidget* parent;
    QFont current_font;
    FontDelegate delegate;

    static bool style_lte(const model::CustomFont& a, const model::CustomFont& b)
    {
        return a.raw_font().weight() < b.raw_font().weight() || (
            a.raw_font().weight() == b.raw_font().weight() &&
            a.raw_font().style() < b.raw_font().style()
        );
    }

    void set_active_font(const QString& family)
    {
        ui.view_style->clear();

        for ( const auto& style : fonts.at(family) )
        {
            auto item = new QListWidgetItem(style.raw_font().styleName());
            ui.view_style->addItem(item);
        }

        current_font.setFamily(family);
        ui.view_style->setCurrentRow(0);
    }

    void loader_finished()
    {
        for ( const auto& font : loader.fonts() )
            fonts[font.family()].push_back(font);

        loader.clear();
        ui.progress_bar->setVisible(false);

        if ( !fonts.empty() )
        {
            ui.widget_data->setEnabled(true);

            for ( auto& p : fonts )
            {
                font_model.appendRow(new QStandardItem(p.first));
                std::sort(p.second.begin(), p.second.end(), &style_lte);
            }

            ui.view_fonts->setCurrentIndex(font_model.index(0, 0));
        }
    }

    void style_selected(const QString& style)
    {
        current_font.setStyleName(style);
        emit parent->font_changed(current_font);
    }
};

glaxnimate::gui::font::ExternalFontWidget::ExternalFontWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    d->ui.setupUi(this);
    d->ui.view_fonts->setModel(&d->font_model);
    d->ui.view_fonts->setItemDelegateForColumn(0, &d->delegate);
    connect(&d->loader, &FontLoader::finished, this, [this]{d->loader_finished();});
    connect(&d->loader, &FontLoader::fonts_queued, d->ui.progress_bar, &QProgressBar::setMaximum);
    connect(&d->loader, &FontLoader::fonts_loaded, d->ui.progress_bar, &QProgressBar::setValue);
    /// \todo handle FontLoader::error
    connect(d->ui.view_style->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& index){
        d->style_selected(index.data().toString());
    });
    connect(d->ui.size_widget, &FontSizeWidget::font_size_changed, this, [this](double size){
        d->current_font.setPointSizeF(size);
        emit font_changed(d->current_font);
    });
    connect(d->ui.view_fonts->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& index){
        d->set_active_font(index.data().toString());
    });
}

glaxnimate::gui::font::ExternalFontWidget::~ExternalFontWidget() = default;

void glaxnimate::gui::font::ExternalFontWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::font::ExternalFontWidget::load_url()
{
    if ( d->url.scheme() == "" )
    {
        d->url = QUrl::fromLocalFile(d->ui.edit_url->text());
        d->ui.edit_url->setText(d->url.toString());
    }

    d->loader.cancel();

    d->loader.queue("", d->url);
    d->ui.progress_bar->setVisible(true);
    d->font_model.clear();
    d->ui.widget_data->setEnabled(false);
    d->ui.view_style->clear();

    d->loader.load_queue();
}

void glaxnimate::gui::font::ExternalFontWidget::url_from_file()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"), {},
        tr("All supported files (*.ttf, *.otf, *.css); CSS files (*.css); Font files (*.ttf, *.otf); All files (*)")
    );

    if ( !path.isEmpty() )
    {
        d->url = QUrl::fromLocalFile(path);
        d->ui.edit_url->setText(d->url.toString());
        load_url();
    }
}

void glaxnimate::gui::font::ExternalFontWidget::url_changed(const QString& url)
{
    d->url = url;
    d->ui.button_load->setEnabled(d->url.isValid());
}

glaxnimate::model::CustomFont glaxnimate::gui::font::ExternalFontWidget::custom_font() const
{
    if ( !d->ui.widget_data->isEnabled() )
        return {};

    auto font_iter = d->fonts.find(d->ui.view_fonts->currentIndex().data().toString());
    if ( font_iter == d->fonts.end() )
        return {};

    int style_index = d->ui.view_style->currentRow();
    if ( style_index < 0 || style_index >= int(font_iter->second.size()) )
        return {};

    return font_iter->second[style_index];
}

void glaxnimate::gui::font::ExternalFontWidget::set_font_size(double size)
{
    d->ui.size_widget->set_font_size(size);
}

const QFont & glaxnimate::gui::font::ExternalFontWidget::selected_font() const
{
    return d->current_font;
}


void glaxnimate::gui::font::ExternalFontWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->ui.progress_bar->setVisible(d->loader.loading());
}
