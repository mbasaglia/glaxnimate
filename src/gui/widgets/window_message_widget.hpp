#ifndef WINDOWMESSAGEWIDGET_H
#define WINDOWMESSAGEWIDGET_H

#include <QWidget>
#include <QAction>
#include <memory>

#include "app/log/log_line.hpp"


class WindowMessageWidget : public QWidget
{
    Q_OBJECT

public:
    struct Message
    {
        QString message;
        app::log::Severity severity = app::log::Warning;
        std::vector<std::unique_ptr<QAction>> actions = {};

        template<class QObj, class Func>
        QAction* add_action(const QIcon& icon, const QString& label, QObj* context, Func functor)
        {
            actions.push_back(std::make_unique<QAction>(icon, label));
            connect(actions.back().get(), &QAction::triggered, context, functor);
            return actions.back().get();
        }
    };

    WindowMessageWidget(QWidget* parent = nullptr);
    ~WindowMessageWidget();

    void queue_message(Message msg);

protected:
    void changeEvent ( QEvent* e ) override;

private:
    void show_message(const Message& msg);

private slots:
    void next_message();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // WINDOWMESSAGEWIDGET_H
