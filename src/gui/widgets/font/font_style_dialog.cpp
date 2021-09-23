#include "font_style_dialog.hpp"
#include "ui_font_style_dialog.h"

#include <QEvent>
#include "app/widgets/no_close_on_enter.hpp"

class glaxnimate::gui::font::FontStyleDialog::Private
{
public:
    Ui::FontStyleDialog ui;
    app::widgets::NoCloseOnEnter ncoe;
};


glaxnimate::gui::font::FontStyleDialog::FontStyleDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    installEventFilter(&d->ncoe);
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

const QFont& glaxnimate::gui::font::FontStyleDialog::font() const
{
    return d->ui.widget_system->font();
}

void glaxnimate::gui::font::FontStyleDialog::set_font(const QFont& font)
{
    d->ui.widget_system->set_font(font);
    d->ui.google_fonts_widget->set_font_size(font.pointSizeF());
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
    d->ui.tab_google->setEnabled(d->ui.google_fonts_widget->model().has_token());
}

glaxnimate::model::CustomFont glaxnimate::gui::font::FontStyleDialog::custom_font() const
{
    if ( d->ui.tab_widget->currentWidget() == d->ui.tab_google )
        return d->ui.google_fonts_widget->custom_font();

    return {};
}
