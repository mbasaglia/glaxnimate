#pragma once

#include <QKeyEvent>

class NoCloseOnEnter : public QObject
{
protected:
    bool eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if ( keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter )
                return true;
        }

        return QObject::eventFilter(obj, event);
    }
};

