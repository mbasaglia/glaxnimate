#include "edit_tool_widget.hpp"
#include "ui_edit_tool_widget.h"

#include <QEvent>

#include "app/settings/settings.hpp"

class EditToolWidget::Private
{
public:
    Ui::EditToolWidget ui;


    void load_settings()
    {
        ui.check_mask->setChecked(app::settings::get<bool>("tools", "edit_mask"));
    }

    void save_settings()
    {
        app::settings::set("tools", "edit_mask", ui.check_mask->isChecked());
    }
};

EditToolWidget::EditToolWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    connect(d->ui.check_mask, &QCheckBox::toggled, this, [this](bool checked){
        show_masks_changed(checked);
        d->save_settings();
    });
    d->load_settings();
}

EditToolWidget::~EditToolWidget() = default;

void EditToolWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

bool EditToolWidget::show_masks() const
{
    return d->ui.check_mask->isChecked();
}
