#include "base_dialog.hpp"


#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QDebug>

glaxnimate::android::BaseDialog::BaseDialog(QWidget *parent)
    : QDialog(parent)
{
    //    setWindowFlags(Qt::Window);
    //    setAttribute(Qt::WA_TranslucentBackground, false);
    //    setWindowState(windowState() | Qt::WindowFullScreen);
#ifndef Q_OS_ANDROID_FAKE
        showMaximized();
#endif
        setVisible(false);
}

int glaxnimate::android::BaseDialog::exec()
{
#ifndef Q_OS_ANDROID_FAKE
    showMaximized();
#endif
    setFocus();
    return QDialog::exec();
}

void glaxnimate::android::BaseDialog::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.fillRect(ev->rect(), palette().window());
    QDialog::paintEvent(ev);

}

void glaxnimate::android::BaseDialog::keyReleaseEvent(QKeyEvent * ev)
{
    QDialog::keyReleaseEvent(ev);

    if ( ev->key() == Qt::Key_Back )
    {
        reject();
        ev->accept();
    }
}

bool glaxnimate::android::BaseDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( event->type() == QEvent::KeyPress )
    {
        auto key_event = static_cast<QKeyEvent*>(event);
        if ( key_event->key() == Qt::Key_Back )
        {
            reject();
            return true;
        }
    }

    return QDialog::eventFilter(object, event);
}

glaxnimate::android::DialogFixerFilter::DialogFixerFilter(QDialog *target)
{
    set_target(target);
}

void glaxnimate::android::DialogFixerFilter::set_target(QDialog *target)
{
    if ( target )
    {
        target->installEventFilter(this);
        bool visible = target->isVisible();
        target->showMaximized();
        if ( !visible )
            target->hide();
    }
}

bool glaxnimate::android::DialogFixerFilter::eventFilter(QObject *object, QEvent *event)
{
    if ( target )
    {
        if ( event->type() == QEvent::KeyRelease )
        {
            auto key_event = static_cast<QKeyEvent*>(event);
            if ( key_event->key() == Qt::Key_Back )
                target->reject();
        }
    }

    return QObject::eventFilter(object, event);
}
