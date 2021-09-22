#include "google_fonts_widget.hpp"
#include "ui_google_fonts_widget.h"

#include <QEvent>
#include <QSortFilterProxyModel>
#include "io/svg/font_weight.hpp"


class glaxnimate::gui::font::GoogleFontsWidget::Private
{
public:
    GoogleFontsWidget* parent;
    Ui::GoogleFontsWidget ui;
    GoogleFontsModel model;
    QSortFilterProxyModel proxy_model;
    QFont current_font;
    int current_weight = 400;
    bool current_italic = false;

    void set_active_font(const GoogleFontsModel::GoogleFont* font)
    {
        ui.view_style->clear();

        QListWidgetItem* best_item = nullptr;
        int best_score = 0;
        const GoogleFontsModel::GoogleFont::Style* best_style = nullptr;

        for ( const auto& style : font->styles )
        {
            auto item = new QListWidgetItem(model.style_name(style));
            item->setData(Qt::UserRole, style.weight);
            item->setData(Qt::UserRole+1, style.italic);
            ui.view_style->addItem(item);
            int score = style.score(current_weight, current_italic);

            if ( score > best_score )
            {
                best_item = item;
                best_score = score;
                best_style = &style;
            }
        }

        current_weight = best_style->weight;
        current_italic = best_style->italic;
        ui.view_style->setCurrentItem(best_item);

        current_font.setFamily(font->family);
        using io::svg::WeightConverter;
        current_font.setWeight(WeightConverter::convert(current_weight, WeightConverter::css, WeightConverter::qt));
        current_font.setItalic(current_italic);

        emit parent->font_changed(current_font);
    }
};

glaxnimate::gui::font::GoogleFontsWidget::GoogleFontsWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->parent = this;
    d->ui.setupUi(this);

    d->proxy_model.setSourceModel(&d->model);
    d->ui.view_google_fonts->setModel(&d->proxy_model);
    d->ui.view_google_fonts->verticalHeader()->setVisible(false);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Family, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Popularity, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Category, QHeaderView::ResizeToContents);
    d->ui.view_google_fonts->horizontalHeader()->setSectionResizeMode(GoogleFontsModel::Column::Status, QHeaderView::Stretch);

    d->proxy_model.setSortRole(GoogleFontsModel::SortRole);
    d->ui.view_google_fonts->setSortingEnabled(true);
    d->ui.view_google_fonts->sortByColumn(GoogleFontsModel::Column::Popularity, Qt::AscendingOrder);

    connect(&d->model, &GoogleFontsModel::max_progress_changed, this, [this](int max){
        d->ui.google_fonts_progress->setMaximum(max);
        d->ui.google_fonts_progress->setVisible(max);
        if ( max == 0 )
            d->ui.label_google_error->setText("");
    });
    connect(&d->model, &GoogleFontsModel::progress_changed, d->ui.google_fonts_progress, &QProgressBar::setValue);
    connect(d->ui.button_google_refresh, &QAbstractButton::clicked, this, [this]{
        d->model.refresh();
        d->ui.label_google_error->setText("");
    });
    connect(&d->model, &GoogleFontsModel::error, d->ui.label_google_error, &QLabel::setText);
    connect(&d->model, &GoogleFontsModel::download_finished, this, [this](int row){
        if ( auto font = d->model.font(row) )
            d->set_active_font(font);
    });

    connect(d->ui.view_google_fonts, &QTableView::clicked, this, [this](const QModelIndex& index){
        int row = d->proxy_model.mapToSource(index).row();
        auto font = d->model.font(row);

        d->ui.view_style->clear();

        if ( !font )
            return;

        if ( font->status == GoogleFontsModel::GoogleFont::Available || font->status == GoogleFontsModel::GoogleFont::Broken )
            d->model.download_font(row);
        else
            d->set_active_font(font);
    });
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
    if ( d->model.has_token() && d->model.rowCount() == 0 )
        d->model.refresh();
}

const glaxnimate::gui::font::GoogleFontsModel & glaxnimate::gui::font::GoogleFontsWidget::model() const
{
    return d->model;
}
