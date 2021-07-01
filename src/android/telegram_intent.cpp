#include "telegram_intent.hpp"


#include <QAndroidIntent>
#include <QtAndroid>
#include <QAndroidJniEnvironment>

#include <QApplication>



TelegramIntent::Result TelegramIntent::send_stickers(const QStringList& filenames, const QStringList& emoji)
{
    QAndroidJniObject generator_name = QAndroidJniObject::fromString(qApp->applicationName());
    QAndroidJniObject messenger(
        "org/mattbas/glaxnimate/jnimessenger/JniMessenger",
        "(Ljava/lang/String;)V",
        generator_name.object<jstring>()
    );

    QAndroidJniObject sticker_file = QAndroidJniObject::fromString(filenames[0]);
    QAndroidJniObject sticker_emoji = QAndroidJniObject::fromString(emoji[0]);
    messenger.callMethod<void>("add_sticker", "(Ljava/lang/String;Ljava/lang/String;)V", sticker_file.object<jstring>(), sticker_emoji.object<jstring>());


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
