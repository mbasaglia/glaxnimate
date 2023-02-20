/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "text_tool_widget.hpp"

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QSpacerItem>
#include <QAbstractItemView>

#include "shape_tool_widget_p.hpp"
#include "widgets/enum_combo.hpp"
#include "widgets/font/font_model.hpp"
#include "widgets/font/font_delegate.hpp"
#include "widgets/font/font_style_dialog.hpp"


class glaxnimate::gui::TextToolWidget::Private : public ShapeToolWidget::Private
{
public:
    const QFont& font() const
    {
        return font_;
    }

    void set_font(const QFont& font)
    {
        QFontInfo info(font);
        font_.setFamily(info.family());
        font_.setStyleName(info.styleName());
        update_styles(info.family());
        combo_font->setCurrentIndex(model.index_for_font(info.family()).row());
        combo_font->setEditText(info.family());
        font_.setPointSizeF(font.pointSizeF());
        spin_size->setValue(font.pointSizeF());
//         combo_style->setCurrentText(info.styleName());
    }

    void on_setup_ui(ShapeToolWidget * p, QVBoxLayout * layout) override
    {
        TextToolWidget* parent = static_cast<TextToolWidget*>(p);

        model.set_font_filters(font::FontModel::ScalableFonts);

        group = new QGroupBox(parent);
        layout->insertWidget(0, group);
        QGridLayout* grid = new QGridLayout();

        // For some reason doing this makes it smaller than adding the grid directly o_O
        QVBoxLayout* l = new QVBoxLayout();
        l->setSpacing(0);
        l->setContentsMargins(0, 0, 0, 0);
        group->setLayout(l);
        l->addLayout(grid);

        int row = 0;


        combo_font = new QComboBox(parent);
        combo_font->setItemDelegate(&delegate);
        combo_font->setModel(&model);
        combo_font->setInsertPolicy(QComboBox::NoInsert);
        combo_font->setEditable(true);
        combo_font->view()->setMinimumWidth(combo_font->view()->sizeHintForColumn(0));
        connect(combo_font,
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                &QComboBox::textActivated,
#else
                QOverload<const QString&>::of(&QComboBox::activated),
#endif
                parent, [this, parent](const QString& family){
            update_styles(family);
            parent->on_font_changed();
        });
        combo_font->setMinimumContentsLength(5);
        grid->addWidget(combo_font, row++, 0, 1, 2);

        label_style = new QLabel(parent);
        grid->addWidget(label_style, row, 0);

        combo_style = new QComboBox(parent);
        connect(combo_style, QOverload<int>::of(&QComboBox::activated), parent, [this, parent]{
            font_.setStyleName(combo_style->currentText());
            parent->on_font_changed();
        });
        grid->addWidget(combo_style, row, 1);
        row++;

        label_size = new QLabel(parent);
        grid->addWidget(label_size, row, 0);

        spin_size = new QDoubleSpinBox(parent);
        spin_size->setMinimum(1);
        spin_size->setMaximum(std::numeric_limits<int>::max());
        spin_size->setSingleStep(0.1);
        connect(spin_size, &QDoubleSpinBox::editingFinished, parent, [this, parent]{
            font_.setPointSizeF(spin_size->value());
            parent->on_font_changed();
        });
        grid->addWidget(spin_size, row, 1);
        row++;

        button = new QPushButton(parent);
        button->setIcon(QIcon::fromTheme("dialog-text-and-font"));
        connect(button, &QPushButton::clicked, parent, [this, parent]{
            dialog->set_favourites(app::settings::get<QStringList>("tools", "favourite_font_families"));
            dialog->set_font(font_);
            bool ok = dialog->exec();

            auto faves = dialog->favourites();
            app::settings::set("tools", "favourite_font_families", faves);
            model.set_favourites(faves);

            if ( ok )
            {
                auto custom = dialog->custom_font();
                if ( custom.is_valid() )
                    emit parent->custom_font_selected(custom.database_index());
                parent->set_font(dialog->selected_font());
            }
        });
        grid->addWidget(button, row++, 0, 1, 2);

//         QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
//         layout->insertItem(3, spacer);

        dialog = new font::FontStyleDialog(parent);

        set_font(QFont("sans", 36));
        on_retranslate();
    }

    void on_load_settings() override
    {
        QFontInfo info(font_);
        model.set_favourites(app::settings::define("tools", "favourite_font_families", QStringList()));
        combo_font->setCurrentText(app::settings::define("tools", "text_family", info.family()));
        combo_font->setEditText(combo_font->currentText());
        font_.setFamily(combo_font->currentText());
        combo_style->setCurrentText(app::settings::define("tools", "text_style", info.styleName()));
        font_.setStyleName(combo_style->currentText());
        spin_size->setValue(app::settings::define("tools", "text_size", info.pointSizeF()));
        font_.setPointSizeF(spin_size->value());
    }

    void on_save_settings() override
    {
        app::settings::set("tools", "text_family", combo_font->currentText());
        app::settings::set("tools", "text_style", combo_style->currentText());
        app::settings::set("tools", "text_size", spin_size->value());
    }

    void on_retranslate() override
    {
        label_style->setText("Style");
        label_size->setText("Size");
        button->setText("Advanced...");
        group->setTitle("Font");
    }

    void update_styles(const QString& family)
    {
        combo_style->clear();
        combo_style->insertItems(0, db.styles(family));
        font_.setFamily(family);
        combo_style->setCurrentText(QFontInfo(font_).styleName());
    }

    QComboBox* combo_font;

    QLabel* label_style;
    QComboBox* combo_style;


    QLabel* label_size;
    QDoubleSpinBox* spin_size;

    QPushButton* button;
    font::FontStyleDialog* dialog;

    QFont font_;
    QFontDatabase db;

    font::FontModel model;
    font::FontDelegate delegate;

    QGroupBox* group;
};


glaxnimate::gui::TextToolWidget::TextToolWidget(QWidget* parent)
    : ShapeToolWidget(std::make_unique<Private>(), parent)
{
}

glaxnimate::gui::TextToolWidget::Private * glaxnimate::gui::TextToolWidget::dd() const
{
    return static_cast<Private*>(d.get());
}


QFont glaxnimate::gui::TextToolWidget::font() const
{
    return dd()->font();
}

void glaxnimate::gui::TextToolWidget::on_font_changed()
{
    save_settings();
    emit font_changed(dd()->font());
}

void glaxnimate::gui::TextToolWidget::set_font(const QFont& font)
{
    dd()->set_font(font);
    emit font_changed(dd()->font());
}

void glaxnimate::gui::TextToolWidget::set_preview_text(const QString& text)
{
    dd()->dialog->set_preview_text(text);
}

void glaxnimate::gui::TextToolWidget::set_document(model::Document* document)
{
    dd()->model.set_document(document);
}
