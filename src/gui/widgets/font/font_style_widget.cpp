#include "font_style_widget.hpp"
#include "ui_font_style_widget.h"

#include <QEvent>
#include <QStringListModel>
#include <QStandardItemModel>

#include "font_model.hpp"
#include "font_delegate.hpp"

class font::FontStyleWidget::Private
{
public:
    void update_from_font()
    {
        info = QFontInfo(font);
        ui.edit_family->setText(info.family());
        ui.view_family->setCurrentIndex(model.index_for_font(info.family()));
        refresh_styles();
        ui.spin_size->setValue(info.pointSizeF());
    }

    void refresh_styles()
    {
        info = QFontInfo(font);
        QStringList styles = database.styles(info.family());
        style_model.setStringList(styles);
        ui.view_style->setCurrentIndex(style_model.index(styles.indexOf(info.styleName()), 0));
    }

    void on_font_changed(FontStyleWidget* parent)
    {
        info = QFontInfo(font);
        emit parent->font_changed(font);
        emit parent->font_edited(font);
    }

    Ui::FontStyleWidget ui;

    FontModel model;
    FontDelegate delegate;

    QFont font;
    QFontInfo info{font};
    QFontDatabase database;

    QList<int> standard_sizes;
    QStandardItemModel sizes_model;

    QStringListModel style_model;
};

font::FontStyleWidget::FontStyleWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->ui.view_family->setModel(&d->model);
    d->ui.view_family->setItemDelegateForColumn(0, &d->delegate);
    d->ui.view_family->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    d->ui.view_family->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    d->standard_sizes = QFontDatabase::standardSizes();
    for ( int s : d->standard_sizes )
        d->sizes_model.appendRow(new QStandardItem(QString::number(s)));
    d->ui.view_size->setModel(&d->sizes_model);

    d->ui.view_style->setModel(&d->style_model);


    for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i)
    {
        QFontDatabase::WritingSystem ws = QFontDatabase::WritingSystem(i);
        QString name = QFontDatabase::writingSystemName(ws);
        if ( name.isEmpty() )
            break;
        d->ui.combo_system->addItem(name, i);
    }

    connect(d->ui.view_family->selectionModel(), &QItemSelectionModel::currentChanged, this, &FontStyleWidget::family_selected);
    connect(d->ui.view_style->selectionModel(), &QItemSelectionModel::currentChanged, this, &FontStyleWidget::style_selected);
    connect(d->ui.view_size->selectionModel(), &QItemSelectionModel::currentChanged, this, &FontStyleWidget::size_selected);

}

font::FontStyleWidget::~FontStyleWidget() = default;

void font::FontStyleWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void font::FontStyleWidget::set_font(const QFont& font)
{
    d->font = font;
    d->update_from_font();
    emit font_changed(d->font);
}

font::FontModel & font::FontStyleWidget::model()
{
    return d->model;
}

const QFont & font::FontStyleWidget::font() const
{
    return d->font;
}

void font::FontStyleWidget::family_edited(const QString& family)
{
    auto index = d->model.index_for_font(family);
    if ( index.isValid() )
    {
        bool b = d->ui.view_family->blockSignals(true);
        d->ui.view_family->setCurrentIndex(index);
        d->ui.view_family->blockSignals(b);

        d->font.setFamily(family);
        d->refresh_styles();
        d->on_font_changed(this);
    }
}

void font::FontStyleWidget::family_selected(const QModelIndex& index)
{
    QString family = index.siblingAtColumn(0).data(Qt::EditRole).toString();
    d->ui.edit_family->setText(family);

    d->font.setFamily(family);
    d->refresh_styles();
    d->on_font_changed(this);
}

void font::FontStyleWidget::filter_flags_changed()
{
    FontModel::FontFilters filters = FontModel::ScalableFonts;
    if ( d->ui.check_monospace->isChecked() )
        filters |= FontModel::MonospacedFonts;
    else if ( d->ui.check_proportional->isChecked() )
        filters |= FontModel::ProportionalFonts;
    d->model.set_font_filters(filters);
    d->ui.view_family->setCurrentIndex(d->model.index_for_font(d->info.family()));
}

void font::FontStyleWidget::size_edited(double size)
{
    bool b = d->ui.view_size->blockSignals(true);

    for ( int i = 0; i < d->standard_sizes.size(); i++ )
        if ( d->standard_sizes[i] == size )
            d->ui.view_size->setCurrentIndex(d->sizes_model.index(i, 0));

    d->ui.view_size->blockSignals(b);

    d->font.setPointSizeF(size);
    d->on_font_changed(this);
}


void font::FontStyleWidget::size_selected(const QModelIndex& index)
{
    bool b = d->ui.spin_size->blockSignals(true);

    qreal size = index.data().toInt();
    d->ui.spin_size->setValue(size);

    d->ui.spin_size->blockSignals(b);

    d->font.setPointSizeF(size);
    d->on_font_changed(this);
}

void font::FontStyleWidget::style_selected(const QModelIndex& index)
{
    QString style = index.data().toString();

    d->font.setStyleName(style);
    d->on_font_changed(this);
}

void font::FontStyleWidget::system_changed(int)
{
    d->model.set_writing_system(QFontDatabase::WritingSystem(d->ui.combo_system->currentData().toInt()));
    d->ui.view_family->setCurrentIndex(d->model.index_for_font(d->info.family()));
}

void font::FontStyleWidget::family_clicked(const QModelIndex& index)
{
    if ( index.column() == 1 )
        d->model.toggle_favourite(index.siblingAtColumn(0).data().toString());
}
