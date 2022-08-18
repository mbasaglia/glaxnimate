#include "google_fonts_widget.hpp"
#include "ui_google_fonts_widget.h"

#include <QEvent>
#include <QSortFilterProxyModel>

#include "io/svg/font_weight.hpp"

namespace  {

class SortFilterGoogleFont : public QSortFilterProxyModel
{
public:

    void set_search(const QString& query)
    {
        search = query.toLower();
        invalidateFilter();
    }

    void set_subset(const QString& subset)
    {
        this->subset = subset;
        invalidateFilter();
    }

    void set_category(glaxnimate::gui::font::GoogleFontsModel::GoogleFont::Category category)
    {
        this->category = category;
        invalidateFilter();
    }


    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if ( source_row < 0 || source_row >= sourceModel()->rowCount(source_parent) )
            return false;

        auto source = static_cast<glaxnimate::gui::font::GoogleFontsModel*>(sourceModel());

        QModelIndex index = source->index(source_row, 0, source_parent);

        if ( !search.isEmpty() )
        {
            if ( !source->data(index, Qt::DisplayRole).toString().toLower().contains(search) )
                return false;
        }

        if ( !subset.isEmpty() )
        {
            if ( !source->has_subset(index, subset) )
                return false;
        }

        if ( category != glaxnimate::gui::font::GoogleFontsModel::GoogleFont::Any )
        {
            if ( !source->has_category(index, category) )
                return false;
        }

        return true;

    }
private:
    QString search;
    QString subset;
    glaxnimate::gui::font::GoogleFontsModel::GoogleFont::Category category = glaxnimate::gui::font::GoogleFontsModel::GoogleFont::Any;
};

} // namespace

class glaxnimate::gui::font::GoogleFontsWidget::Private
{
public:
    GoogleFontsWidget* parent;
    Ui::GoogleFontsWidget ui;
    GoogleFontsModel model;
    SortFilterGoogleFont proxy_model;
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
            auto item = new QListWidgetItem(style.font.style_name());
//             item->setData(Qt::UserRole, style.weight);
//             item->setData(Qt::UserRole+1, style.italic);
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
        current_font.setWeight(QFont::Weight(WeightConverter::convert(current_weight, WeightConverter::css, WeightConverter::qt)));
        current_font.setItalic(current_italic);

        emit parent->font_changed(current_font);
    }

    void update_writing_systems()
    {
        auto current = ui.combo_system->currentText();
        ui.combo_system->clear();
        ui.combo_system->addItem("");
        for ( const auto& subset : model.subsets() )
            ui.combo_system->addItem(subset);
        ui.combo_system->setCurrentText(current);
    }

    void add_category(GoogleFontsModel::GoogleFont::Category cat)
    {
        ui.combo_category->addItem(model.category_name(cat), int(cat));
    }

    void add_categories()
    {
        add_category(GoogleFontsModel::GoogleFont::Any);
        add_category(GoogleFontsModel::GoogleFont::Serif);
        add_category(GoogleFontsModel::GoogleFont::SansSerif);
        add_category(GoogleFontsModel::GoogleFont::Monospace);
        add_category(GoogleFontsModel::GoogleFont::Display);
        add_category(GoogleFontsModel::GoogleFont::Handwriting);
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
        d->ui.button_google_refresh->setEnabled(max == 0);
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

    connect(d->ui.view_style->selectionModel(), &QItemSelectionModel::currentChanged, this, &GoogleFontsWidget::change_style);

    connect(&d->model, &GoogleFontsModel::refresh_finished, this, [this](){
        d->update_writing_systems();
    });
    connect(d->ui.combo_system, &QComboBox::currentTextChanged, &d->proxy_model, &SortFilterGoogleFont::set_subset);
    connect(d->ui.edit_family, &QLineEdit::textChanged, &d->proxy_model, &SortFilterGoogleFont::set_search);

    d->add_categories();
    connect(d->ui.combo_category, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](){
        d->proxy_model.set_category(GoogleFontsModel::GoogleFont::Category(d->ui.combo_category->currentData().toInt()));
    });

    connect(d->ui.size_widget, &FontSizeWidget::font_size_changed, this, [this](double size){
        d->current_font.setPointSizeF(size);
        emit font_changed(d->current_font);
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

void glaxnimate::gui::font::GoogleFontsWidget::change_style(const QModelIndex& index)
{
    if ( index.isValid() )
    {
        d->current_font.setStyleName(index.data(Qt::DisplayRole).toString());
        emit font_changed(d->current_font);
    }
}

void glaxnimate::gui::font::GoogleFontsWidget::set_font_size(double size)
{
    d->ui.size_widget->set_font_size(size);
}

glaxnimate::model::CustomFont glaxnimate::gui::font::GoogleFontsWidget::custom_font() const
{
    int font_row = d->proxy_model.mapToSource(d->ui.view_google_fonts->currentIndex()).row();
    auto font = d->model.font(font_row);
    if ( !font )
        return {};

    auto style_row = d->ui.view_style->currentRow();
    if ( style_row < 0 || style_row >= int(font->styles.size()) )
        return {};

    return font->styles[style_row].font;
}

const QFont & glaxnimate::gui::font::GoogleFontsWidget::selected_font() const
{
    return d->current_font;
}
