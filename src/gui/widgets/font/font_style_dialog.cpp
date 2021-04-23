#include "font_style_dialog.hpp"
#include "ui_font_style_dialog.h"

#include <QEvent>

#include "style/no_close_on_enter.hpp"

class font::FontStyleDialog::Private
{
public:
    Ui::FontStyleDialog ui;
    NoCloseOnEnter ncoe;
};


font::FontStyleDialog::FontStyleDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    installEventFilter(&d->ncoe);
}

font::FontStyleDialog::~FontStyleDialog() = default;

void font::FontStyleDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

const QFont& font::FontStyleDialog::font() const
{
    return d->ui.widget_system->font();
}

void font::FontStyleDialog::set_font(const QFont& font)
{
    d->ui.widget_system->set_font(font);
}

void font::FontStyleDialog::set_preview_text(const QString& text)
{
    d->ui.preview->set_text(text);
}

QStringList font::FontStyleDialog::favourites() const
{
    return d->ui.widget_system->model().favourites();
}

void font::FontStyleDialog::set_favourites(const QStringList& fav)
{
    d->ui.widget_system->model().set_favourites(fav);
}

