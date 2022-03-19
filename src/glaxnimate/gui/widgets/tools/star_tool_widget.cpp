#include "star_tool_widget.hpp"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGridLayout>

#include "shape_tool_widget_p.hpp"
#include "widgets/enum_combo.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;

class StarToolWidget::Private : public ShapeToolWidget::Private
{
public:
    model::PolyStar::StarType star_type() const
    {
        return model::PolyStar::StarType(combo->current_value());
    }

    double spoke_ratio() const
    {
        return spin_ratio->value();
    }

    int points() const
    {
        return spin_points->value();
    }

protected:
    void on_setup_ui(ShapeToolWidget * parent, QVBoxLayout * layout) override
    {
        group = new QGroupBox(parent);
        layout->insertWidget(0, group);
        QGridLayout* grid = new QGridLayout();
        group->setLayout(grid);
        int row = 0;

        combo = new EnumCombo(model::PolyStar::Star, parent);
        grid->addWidget(combo, row++, 0, 1, 2);
        connect(combo, QOverload<int>::of(&QComboBox::activated), parent, &StarToolWidget::save_settings);


        label_ratio = new QLabel(parent);
        grid->addWidget(label_ratio, row, 0);

        spin_ratio = new QDoubleSpinBox(parent);
        spin_ratio->setMinimum(0);
        spin_ratio->setMaximum(1);
        spin_ratio->setSingleStep(0.1);
        connect(spin_ratio, &QDoubleSpinBox::editingFinished, parent, &StarToolWidget::save_settings);
        grid->addWidget(spin_ratio, row, 1);
        row++;

        label_points = new QLabel(parent);
        grid->addWidget(label_points, row, 0);

        spin_points = new QSpinBox(parent);
        spin_points->setMinimum(3);
        spin_points->setMaximum(16);
        connect(spin_ratio, &QSpinBox::editingFinished, parent, &StarToolWidget::save_settings);
        grid->addWidget(spin_points, row, 1);
        row++;

        on_retranslate();
    }

    void on_load_settings() override
    {
        combo->set_current_value(app::settings::get<int>("tools", "star_type", 0));
        spin_ratio->setValue(app::settings::get<double>("tools", "star_ratio", 0.5));
        spin_points->setValue(app::settings::get<int>("tools", "star_points", 5));
    }

    void on_save_settings() override
    {
        app::settings::set("tools", "star_type", combo->current_value());
        app::settings::set("tools", "star_ratio", spin_ratio->value());
        app::settings::set("tools", "star_points", spin_points->value());
    }

    void on_retranslate() override
    {
        label_ratio->setText("Spoke Ratio");
        label_points->setText("Corners");
        group->setTitle("Star");
    }

    EnumCombo* combo;
    QLabel* label_ratio;
    QDoubleSpinBox* spin_ratio;
    QLabel* label_points;
    QSpinBox* spin_points;
    QGroupBox* group;
};


StarToolWidget::StarToolWidget(QWidget* parent)
    : ShapeToolWidget(std::make_unique<Private>(), parent)
{
}

model::PolyStar::StarType StarToolWidget::star_type() const
{
    return static_cast<Private*>(d.get())->star_type();
}

double StarToolWidget::spoke_ratio() const
{
    return static_cast<Private*>(d.get())->spoke_ratio();
}


int StarToolWidget::points()
{
    return static_cast<Private*>(d.get())->points();
}
