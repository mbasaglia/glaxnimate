/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "plugin_settings_widget.hpp"
#include "ui_plugin_settings_widget.h"

#include "plugin/plugin.hpp"

#include <QEvent>

using namespace glaxnimate::gui;
using namespace glaxnimate;

PluginSettingsWidget::PluginSettingsWidget(QWidget* parent)
    : QWidget(parent), d ( std::make_unique<Ui::PluginSettingsWidget>() )
{
    d->setupUi ( this );
    d->list_services->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    update_entries();
}

PluginSettingsWidget::~PluginSettingsWidget() = default;

void PluginSettingsWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
        update_entries();
    }
}

void PluginSettingsWidget::current_changed ( QListWidgetItem* item )
{
    d->stacked_widget->setCurrentWidget(d->page_noplugin);

    if ( !item )
        return;

    current = plugin::PluginRegistry::instance().plugin(item->data(Qt::UserRole).toString());
    if ( !current )
        return;

    bool checked = item->checkState() == Qt::Checked;
    if ( checked != current->enabled() )
    {
        if ( checked )
            current->enable();
        else
            current->disable();
    }

    d->widget_plugin->setTitle(current->data().name);
    d->line_plugin_path->setText(current->data().dir.absolutePath());
    d->line_version->setText(QString::number(current->data().version));
    d->line_author->setText(current->data().author);
    d->btn_disable->setEnabled(current->can_disable());
    d->btn_enable->setEnabled(current->can_enable());
    d->btn_uninstall->setEnabled(current->user_installed());
    d->description->setPlainText(current->data().description);

    d->list_services->clearContents();
    d->list_services->setRowCount(current->data().services.size());

    int row = 0;
    for ( const auto& svc : current->data().services )
    {
        QString type = tr("Unknown");
        if ( svc->type() == plugin::ServiceType::Action )
            type = tr("Menu Action");
        else if ( svc->type() == plugin::ServiceType::IoFormat )
            type = tr("File Format");

        QTableWidgetItem* it;

        it = new QTableWidgetItem(svc->service_icon(), type);
        it->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        d->list_services->setItem(row, 0, it);
        it = new QTableWidgetItem(svc->name());
        it->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        d->list_services->setItem(row, 1, it);
        row++;
    }


    d->stacked_widget->setCurrentWidget(d->page_plugin);

}

void PluginSettingsWidget::disable_current()
{
    if ( current )
    {
        current->disable();
        update_entries();
    }
}

void PluginSettingsWidget::enable_current()
{
    if ( current )
    {
        current->enable();
        update_entries();
    }
}

void PluginSettingsWidget::install_dialog()
{
    /// @todo
}

void PluginSettingsWidget::refresh_plugins()
{
    clear_selection();
    plugin::PluginRegistry::instance().load();
    update_entries();
}

void PluginSettingsWidget::uninstall_current()
{
    /// @todo
}

void PluginSettingsWidget::update_entries()
{
    clear_selection();

    QString current_id;
    if ( auto item = d->list_plugins->currentItem() )
    {
        current_id = item->data(Qt::UserRole).toString();
    }

    QListWidgetItem* current_item = nullptr;

    d->list_plugins->blockSignals(true);
    d->list_plugins->clear();

    for ( const auto& plugin : plugin::PluginRegistry::instance().plugins() )
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setIcon(plugin->icon());
        item->setText(plugin->data().name);
        item->setCheckState(plugin->enabled() ? Qt::Checked : Qt::Unchecked );
        item->setData(Qt::UserRole, plugin->data().id);
        Qt::ItemFlags flags = Qt::ItemIsSelectable;
        if ( plugin->available() )
            flags |= Qt::ItemIsEnabled|Qt::ItemIsUserCheckable;
        item->setFlags(flags);
        item->setData(Qt::ToolTipRole, plugin->data().description);

        d->list_plugins->addItem(item);

        if ( plugin->data().id == current_id )
        {
            d->list_plugins->setCurrentItem(item);
            current_item = item;
        }

    }

    if ( !current_item )
        current_item = d->list_plugins->currentItem();

    d->list_plugins->blockSignals(false);
    current_changed(current_item);

}

void PluginSettingsWidget::clear_selection()
{
    current = nullptr;
    d->stacked_widget->setCurrentWidget(d->page_noplugin);
}

