/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "telegram_intent.hpp"

#ifndef Q_OS_ANDROID_FAKE

#include <QAndroidIntent>
#include <QtAndroid>
#include <QAndroidJniEnvironment>

#include <QApplication>



glaxnimate::android::TelegramIntent::Result glaxnimate::android::TelegramIntent::send_stickers(const QStringList& filenames, const QStringList& emoji)
{
    QAndroidJniObject generator_name = QAndroidJniObject::fromString(qApp->applicationName());
    QAndroidJniObject messenger(
        "org/mattbas/glaxnimate/jnimessenger/JniMessenger",
        "(Ljava/lang/String;)V",
        generator_name.object<jstring>()
    );

    for ( int i = 0; i < filenames.size(); i++ )
    {
        QAndroidJniObject sticker_file = QAndroidJniObject::fromString(filenames[i]);
        QAndroidJniObject sticker_emoji = QAndroidJniObject::fromString(emoji[i]);
        messenger.callMethod<void>("add_sticker", "(Ljava/lang/String;Ljava/lang/String;)V", sticker_file.object<jstring>(), sticker_emoji.object<jstring>());
    }


    QAndroidJniObject intent = messenger.callObjectMethod("import_stickers", "()Landroid/content/Intent;");
    QAndroidJniObject activity = QtAndroid::androidActivity();
    {
        QAndroidJniEnvironment env;
        activity.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object<jobject>());

        if (env->ExceptionCheck())
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return QObject::tr("Could not start activity, is Telegram installed?");
        }
    }
    return {};
}

#else
glaxnimate::android::TelegramIntent::Result glaxnimate::android::TelegramIntent::send_stickers(const QStringList& filenames, const QStringList& emoji)
{
    return {};
}
#endif
