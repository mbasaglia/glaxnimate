#include "toolbar_settings_widget.hpp"
#include "ui_toolbar_settings_widget.h"
#include <QEvent>

#include "settings/toolbar_settings.hpp"

class ToolbarSettingsWidget::Private
{
public:
    Ui::ToolbarSettingsWidget ui;
};

ToolbarSettingsWidget::ToolbarSettingsWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->ui.combo_style->setCurrentIndex(int(settings::ToolbarSettingsGroup::button_style));
    d->ui.spin_icon_size->setValue(settings::ToolbarSettingsGroup::icon_size_extent);
    d->ui.spin_tool_icon_size->setValue(settings::ToolbarSettingsGroup::tool_icon_size_extent);
}

ToolbarSettingsWidget::~ToolbarSettingsWidget() = default;

void ToolbarSettingsWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ToolbarSettingsWidget::update_preview()
{
    settings::ToolbarSettingsGroup::icon_size_extent = d->ui.spin_icon_size->value();
    settings::ToolbarSettingsGroup::tool_icon_size_extent = d->ui.spin_tool_icon_size->value();
    settings::ToolbarSettingsGroup::button_style = Qt::ToolButtonStyle(d->ui.combo_style->currentIndex());
    settings::ToolbarSettingsGroup::apply();

    for ( auto button : findChildren<QToolButton*>() )
    {
        button->setToolButtonStyle(settings::ToolbarSettingsGroup::button_style);
        if ( button->objectName().contains("tool") )
            button->setIconSize(settings::ToolbarSettingsGroup::tool_icon_size());
        else
            button->setIconSize(settings::ToolbarSettingsGroup::icon_size());
    }
}
