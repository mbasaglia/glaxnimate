/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "font_style_widget.hpp"
#include "ui_font_style_widget.h"

#include <QEvent>
#include <QStringListModel>

#include "font_model.hpp"
#include "font_delegate.hpp"


class glaxnimate::gui::font::FontStyleWidget::Private
{
public:
    void update_from_font()
    {
        info = QFontInfo(font);
        ui.edit_family->setText(info.family());
        ui.view_family->setCurrentIndex(model.index_for_font(info.family()));
        refresh_styles();
        ui.size_widget->set_font_size(info.pointSizeF());
    }

    void refresh_styles()
    {
        info = QFontInfo(font);
#if QT_VERSION_MAJOR < 6
        QStringList styles = database.styles(info.family());
#else
        QStringList styles = QFontDatabase::styles(info.family());
#endif
        style_model.setStringList(styles);
        ui.view_style->setCurrentIndex(style_model.index(styles.indexOf(info.styleName()), 0));
    }

    void on_font_changed(FontStyleWidget* parent)
    {
        info = QFontInfo(font);
        Q_EMIT parent->font_changed(font);
        Q_EMIT parent->font_edited(font);
    }

    Ui::FontStyleWidget ui;

    FontModel model;
    FontDelegate delegate;

    QFont font;
    QFontInfo info{font};

#if QT_VERSION_MAJOR < 6
    QFontDatabase database;
#endif

    QStringListModel style_model;
};

glaxnimate::gui::font::FontStyleWidget::FontStyleWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->ui.view_family->setModel(&d->model);
    d->ui.view_family->setItemDelegateForColumn(0, &d->delegate);
    d->ui.view_family->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    d->ui.view_family->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

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
    connect(d->ui.size_widget, &FontSizeWidget::font_size_changed, this, [this](qreal size){
        d->font.setPointSizeF(size);
        d->on_font_changed(this);
    });
}

glaxnimate::gui::font::FontStyleWidget::~FontStyleWidget() = default;

void glaxnimate::gui::font::FontStyleWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::font::FontStyleWidget::set_font(const QFont& font)
{
    d->font = font;
    d->update_from_font();
    Q_EMIT font_changed(d->font);
}

glaxnimate::gui::font::FontModel & glaxnimate::gui::font::FontStyleWidget::model()
{
    return d->model;
}

const QFont & glaxnimate::gui::font::FontStyleWidget::selected_font() const
{
    return d->font;
}

void glaxnimate::gui::font::FontStyleWidget::family_edited(const QString& family)
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

void glaxnimate::gui::font::FontStyleWidget::family_selected(const QModelIndex& index)
{
    QString family = index.siblingAtColumn(0).data(Qt::EditRole).toString();
    d->ui.edit_family->setText(family);

    d->font.setFamily(family);
    d->refresh_styles();
    d->on_font_changed(this);
}

void glaxnimate::gui::font::FontStyleWidget::filter_flags_changed()
{
    FontModel::FontFilters filters = FontModel::ScalableFonts;
    if ( d->ui.check_monospace->isChecked() )
        filters |= FontModel::MonospacedFonts;
    else if ( d->ui.check_proportional->isChecked() )
        filters |= FontModel::ProportionalFonts;
    d->model.set_font_filters(filters);
    d->ui.view_family->setCurrentIndex(d->model.index_for_font(d->info.family()));
}

void glaxnimate::gui::font::FontStyleWidget::style_selected(const QModelIndex& index)
{
    QString style = index.data().toString();

    d->font.setStyleName(style);
    d->on_font_changed(this);
}

void glaxnimate::gui::font::FontStyleWidget::system_changed(int)
{
    d->model.set_writing_system(QFontDatabase::WritingSystem(d->ui.combo_system->currentData().toInt()));
    d->ui.view_family->setCurrentIndex(d->model.index_for_font(d->info.family()));
}

void glaxnimate::gui::font::FontStyleWidget::family_clicked(const QModelIndex& index)
{
    if ( index.column() == 1 )
        d->model.toggle_favourite(index.siblingAtColumn(0).data().toString());
}
