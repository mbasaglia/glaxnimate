#include "window_message_widget.hpp"
#include "ui_window_message_widget.h"

#include <queue>

#include <QPushButton>

class WindowMessageWidget::Private
{
public:
    Ui::WindowMessageWidget ui;
    std::queue<Message> messages;
};

WindowMessageWidget::WindowMessageWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<WindowMessageWidget::Private>())
{
    d->ui.setupUi(this);
    hide();
}
WindowMessageWidget::~WindowMessageWidget() = default;

void WindowMessageWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void WindowMessageWidget::queue_message(WindowMessageWidget::Message msg)
{
    d->messages.emplace(std::move(msg));
    if ( d->messages.size() == 1 )
    {
        show_message(d->messages.front());
        show();
    }
}

void WindowMessageWidget::next_message()
{
    d->messages.pop();
    if ( d->messages.empty() )
        hide();
}

void WindowMessageWidget::show_message(const WindowMessageWidget::Message& msg)
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
        connect(btn, &QPushButton::clicked, action.get(), &QAction::trigger);
        connect(action.get(), &QObject::destroyed, btn, &QObject::deleteLater);
    }
}



