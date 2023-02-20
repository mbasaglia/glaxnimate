/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "window_message_widget.hpp"
#include "ui_window_message_widget.h"

#include <queue>

#include <QPushButton>

using namespace glaxnimate::gui;

class glaxnimate::gui::WindowMessageWidget::Private
{
public:
    Ui::WindowMessageWidget ui;
    std::queue<Message> messages;
};

glaxnimate::gui::WindowMessageWidget::WindowMessageWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<WindowMessageWidget::Private>())
{
    d->ui.setupUi(this);
    hide();
}
glaxnimate::gui::WindowMessageWidget::~WindowMessageWidget() = default;

void glaxnimate::gui::WindowMessageWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::WindowMessageWidget::queue_message(WindowMessageWidget::Message msg)
{
    d->messages.emplace(std::move(msg));
    if ( d->messages.size() == 1 )
    {
        show_message(d->messages.front());
        show();
    }
}

void glaxnimate::gui::WindowMessageWidget::next_message()
{
    d->messages.pop();
    if ( d->messages.empty() )
        hide();
    else
        show_message(d->messages.front());
}

void glaxnimate::gui::WindowMessageWidget::show_message(const WindowMessageWidget::Message& msg)
{
    d->ui.label_message->setText(msg.message);

    int icon_extent = d->ui.label_icon->width();
    switch ( msg.severity )
    {
        case app::log::Info:
            if ( !msg.actions.empty() && msg.message.endsWith("?") )
                d->ui.label_icon->setPixmap(QIcon::fromTheme("dialog-question").pixmap(icon_extent));
            else
                d->ui.label_icon->setPixmap(QIcon::fromTheme("dialog-information").pixmap(icon_extent));
            break;
        case app::log::Warning:
            d->ui.label_icon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(icon_extent));
            break;
        case app::log::Error:
            d->ui.label_icon->setPixmap(QIcon::fromTheme("dialog-error").pixmap(icon_extent));
            break;
    }

    for ( const auto& action : msg.actions )
    {
        auto btn = new QPushButton(this);
        btn->setIcon(action->icon());
        btn->setText(action->text());
        btn->setToolTip(action->toolTip());
        d->ui.lay_buttons->addWidget(btn);
        connect(btn, &QPushButton::clicked, action.get(), &QAction::trigger);
        connect(btn, &QPushButton::clicked, this, &WindowMessageWidget::next_message);
        connect(action.get(), &QObject::destroyed, btn, &QObject::deleteLater);
    }
}



