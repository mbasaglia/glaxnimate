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
        showMaximized();
        setVisible(false);
}

int glaxnimate::android::BaseDialog::exec()
{
    showMaximized();
    setFocus();
    return QDialog::exec();
}

void glaxnimate::android::BaseDialog::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.fillRect(ev->rect(), palette().window());
    QDialog::paintEvent(ev);

}

void glaxnimate::android::BaseDialog::keyPressEvent(QKeyEvent * ev)
{
    if ( ev->key() == Qt::Key_Back )
    {
        reject();
        ev->accept();
    }

    qDebug() << ev->key() << Qt::Key_Back;
    QDialog::keyPressEvent(ev);
}

bool glaxnimate::android::BaseDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( event->type() == QEvent::KeyPress )
    {
        auto key_event = static_cast<QKeyEvent*>(event);
        qDebug() << "filter" << key_event->key();
        if ( key_event->key() == Qt::Key_Back )
        {
            reject();
            return true;
        }
    }

    return QDialog::eventFilter(object, event);
}
