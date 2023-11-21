/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WINDOWMESSAGEWIDGET_H
#define WINDOWMESSAGEWIDGET_H

#include <KMessageWidget>
#include <QAction>
#include <memory>

#include "app/log/log_line.hpp"

namespace glaxnimate::gui {

class WindowMessageWidget : public KMessageWidget
{
    Q_OBJECT

public:
    struct Message
    {
        QString message;
        KMessageWidget::MessageType severity = KMessageWidget::Warning;
        std::vector<std::unique_ptr<QAction>> actions = {};

        template<class QObj, class Func>
        QAction* add_action(const QIcon& icon, const QString& label, QObj* context, Func functor)
        {
            actions.push_back(std::make_unique<QAction>(icon, label));
            connect(actions.back().get(), &QAction::triggered, context, functor);
            return actions.back().get();
        }

        QAction* add_action(const QIcon& icon, const QString& label)
        {
            actions.push_back(std::make_unique<QAction>(icon, label));
            return actions.back().get();
        }
    };

    WindowMessageWidget(QWidget* parent = nullptr);
    ~WindowMessageWidget();

    void queue_message(Message msg);

private:
    void show_message(const Message& msg);

private Q_SLOTS:
    void next_message();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
#endif // WINDOWMESSAGEWIDGET_H
