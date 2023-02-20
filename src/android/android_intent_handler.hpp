/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ANDROID_INTENT_HANDLER_HPP
#define ANDROID_INTENT_HANDLER_HPP

#include <QObject>
#include <QUrl>
#include <QMutex>


namespace glaxnimate::android {

class AndroidIntentHandler : public QObject
{
    Q_OBJECT

public:
    static AndroidIntentHandler* instance();
    void set_view_uri(const QUrl& uri);
    QUrl view_uri();

signals:
    void view_uri_changed(QUrl url);

private:
    AndroidIntentHandler();

    QMutex mutex;
    QUrl uri;
};

} // namespace glaxnimate::android

#endif // ANDROID_INTENT_HANDLER_HPP
