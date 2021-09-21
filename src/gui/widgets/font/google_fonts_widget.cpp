#include "google_fonts_widget.hpp"
#include "ui_google_fonts_widget.h"

#include <QEvent>
#include <QSortFilterProxyModel>


class glaxnimate::gui::font::GoogleFontsWidget::Private
{
public:
    Ui::GoogleFontsWidget ui;
    GoogleFontsModel google_fonts_model;
    QSortFilterProxyModel google_fonts_proxy_model;

};

glaxnimate::gui::font::GoogleFontsWidget::GoogleFontsWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->google_fonts_proxy_model.setSourceModel(&d->google_fonts_model);
    d->ui.view_google_fonts->setModel(&d->google_fonts_proxy_model);
    d->ui.view_google_fonts->verticalHeader()->setVisible(false);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Family, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Popularity, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Category, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Status, QHeaderView::Stretch);
    d->ui.view_google_fonts->setSortingEnabled(true);
    d->ui.view_google_fonts->sortByColumn(GoogleFontsModel::Column::Popularity, Qt::AscendingOrder);
    connect(&d->google_fonts_model, &GoogleFontsModel::max_progress_changed, d->ui.google_fonts_progress, &QProgressBar::setMaximum);
    connect(&d->google_fonts_model, &GoogleFontsModel::progress_changed, d->ui.google_fonts_progress, &QProgressBar::setValue);
    connect(&d->google_fonts_model, &GoogleFontsModel::max_progress_changed, d->ui.google_fonts_progress, &QProgressBar::setVisible);
    connect(d->ui.button_google_refresh, &QAbstractButton::clicked, &d->google_fonts_model, &GoogleFontsModel::refresh);
}

glaxnimate::gui::font::GoogleFontsWidget::~GoogleFontsWidget() = default;

void glaxnimate::gui::font::GoogleFontsWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::font::GoogleFontsWidget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if ( d->google_fonts_model.has_token() && d->google_fonts_model.rowCount() == 0 )
        d->google_fonts_model.refresh();
}

const glaxnimate::gui::font::GoogleFontsModel & glaxnimate::gui::font::GoogleFontsWidget::model() const
{
    return d->google_fonts_model;
}
