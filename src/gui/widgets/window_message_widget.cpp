/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "window_message_widget.hpp"

#include <queue>

#include <QPushButton>

using namespace glaxnimate::gui;

class glaxnimate::gui::WindowMessageWidget::Private
{
public:
    std::queue<Message> messages;
};

glaxnimate::gui::WindowMessageWidget::WindowMessageWidget(QWidget* parent)
    : KMessageWidget(parent), d(std::make_unique<WindowMessageWidget::Private>())
{
    setCloseButtonVisible(true);
    hide();
    connect(this, &KMessageWidget::hideAnimationFinished, this, &WindowMessageWidget::next_message);
}
glaxnimate::gui::WindowMessageWidget::~WindowMessageWidget() = default;

void glaxnimate::gui::WindowMessageWidget::queue_message(WindowMessageWidget::Message msg)
{
    d->messages.emplace(std::move(msg));
    if ( d->messages.size() == 1 )
    {
        show_message(d->messages.front());
    }
}

void glaxnimate::gui::WindowMessageWidget::next_message()
{
    // clearActions();
    d->messages.pop();
    if ( d->messages.empty() )
        hide();
    else
        show_message(d->messages.front());
}

void glaxnimate::gui::WindowMessageWidget::show_message(const WindowMessageWidget::Message& msg)
{
    setMessageType(msg.severity);
    setText(msg.message);

    for ( const auto& action : msg.actions )
        addAction(action.get());

    switch ( msg.severity )
    {
        case KMessageWidget::Information:
            if ( !msg.actions.empty() && msg.message.endsWith("?") )
                setIcon(QIcon::fromTheme("dialog-question"));
            else
                setIcon(QIcon::fromTheme("dialog-information"));
            break;
        case KMessageWidget::Warning:
            setIcon(QIcon::fromTheme("dialog-warning"));
            break;
        case KMessageWidget::Error:
            setIcon(QIcon::fromTheme("dialog-error"));
            break;
        case KMessageWidget::Positive:
            setIcon(QIcon::fromTheme("dialog-ok"));
            break;
    }

    animatedShow();
}
