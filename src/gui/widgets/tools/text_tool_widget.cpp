#include "text_tool_widget.hpp"

#include <QLabel>
#include <QFontComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QFontDialog>
#include <QSpacerItem>

#include <QDebug>
#include "shape_tool_widget_p.hpp"
#include "widgets/enum_combo.hpp"

class TextToolWidget::Private : public ShapeToolWidget::Private
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
        combo_font->setCurrentText(info.family());
        font_.setPointSizeF(info.pointSizeF());
        spin_size->setValue(info.pointSizeF());
    }

private:
    void on_setup_ui(ShapeToolWidget * p, QVBoxLayout * layout) override
    {
        TextToolWidget* parent = static_cast<TextToolWidget*>(p);

        combo_font = new QFontComboBox(parent);
        layout->insertWidget(0, combo_font);
        connect(combo_font, &QComboBox::currentTextChanged, parent, [this, parent](const QString& family){
            update_styles(family);
            parent->on_font_changed();
        });


        QGridLayout* grid = new QGridLayout();
        layout->insertLayout(1, grid);

        label_style = new QLabel(parent);
        grid->addWidget(label_style, 0, 0);

        combo_style = new QComboBox(parent);
        connect(combo_style, QOverload<int>::of(&QComboBox::activated), parent, [this, parent]{
            font_.setStyleName(combo_style->currentText());
            parent->on_font_changed();
        });
        grid->addWidget(combo_style, 0, 1);

        label_size = new QLabel(parent);
        grid->addWidget(label_size, 1, 0);

        spin_size = new QDoubleSpinBox(parent);
        spin_size->setMinimum(1);
        spin_size->setMaximum(std::numeric_limits<int>::max());
        spin_size->setSingleStep(0.1);
        connect(spin_size, &QDoubleSpinBox::editingFinished, parent, [this, parent]{
            font_.setPointSizeF(spin_size->value());
            parent->on_font_changed();
        });
        grid->addWidget(spin_size, 1, 1);

        button = new QPushButton(parent);
        button->setIcon(QIcon::fromTheme("dialog-text-and-font"));
        connect(button, &QPushButton::clicked, parent, [this, parent]{
            dialog->setCurrentFont(font_);
            if ( dialog->exec() )
                parent->set_font(dialog->currentFont());
        });
        layout->insertWidget(2, button);

        QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        layout->insertItem(3, spacer);


        dialog = new QFontDialog(parent);
        dialog->setOptions(QFontDialog::ScalableFonts|QFontDialog::MonospacedFonts|QFontDialog::ProportionalFonts);

        set_font(QFont("sans", 32));
        on_retranslate();
    }

    void on_load_settings() override
    {
        combo_font->setCurrentText(app::settings::get<QString>("tools", "text_family", font_.family()));
        combo_style->setCurrentText(app::settings::get<QString>("tools", "text_style", font_.styleName()));
        spin_size->setValue(app::settings::get<qreal>("tools", "text_size", font_.pointSizeF()));
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
    }

    void update_styles(const QString& family)
    {
        combo_style->clear();
        combo_style->insertItems(0, db.styles(family));
        font_.setFamily(family);
        combo_style->setCurrentText(QFontInfo(font_).styleName());
    }

    QFontComboBox* combo_font;

    QLabel* label_style;
    QComboBox* combo_style;


    QLabel* label_size;
    QDoubleSpinBox* spin_size;

    QPushButton* button;
    QFontDialog* dialog;

    QFont font_;
    QFontDatabase db;
};


TextToolWidget::TextToolWidget(QWidget* parent)
    : ShapeToolWidget(std::make_unique<Private>(), parent)
{
}

TextToolWidget::Private * TextToolWidget::dd() const
{
    return static_cast<Private*>(d.get());
}


QFont TextToolWidget::font() const
{
    return dd()->font();
}

void TextToolWidget::on_font_changed()
{
    save_settings();
    emit font_changed(dd()->font());
}

void TextToolWidget::set_font(const QFont& font)
{
    dd()->set_font(font);
    emit font_changed(dd()->font());
}

void TextToolWidget::showEvent(QShowEvent* event)
{
    ShapeToolWidget::showEvent(event);
    // dunno why but some widgets clear their contents when shown
//     dd()->set_font(dd()->font());
}
