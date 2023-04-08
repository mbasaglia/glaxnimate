/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "android_intent_handler.hpp"

#ifdef Q_OS_ANDROID

extern "C" {
#include <jni.h>

JNIEXPORT void JNICALL Java_org_mattbas_glaxnimate_GlaxnimateActivity_openIntent(
    JNIEnv *env, jobject obj, jstring uri)
{
    Q_UNUSED(obj);
    const char* utf = env->GetStringUTFChars(uri, nullptr);
    QUrl cppuri(utf);
    env->ReleaseStringUTFChars(uri, utf);
    glaxnimate::android::AndroidIntentHandler::instance()->set_view_uri(cppuri);
}

} // extern C
#endif

glaxnimate::android::AndroidIntentHandler *glaxnimate::android::AndroidIntentHandler::instance()
{
    static AndroidIntentHandler instance;
    return &instance;
}

void glaxnimate::android::AndroidIntentHandler::set_view_uri(const QUrl &uri)
{
    QMutexLocker lock(&mutex);
    this->uri = uri;
    emit view_uri_changed(uri);
}

QUrl glaxnimate::android::AndroidIntentHandler::view_uri()
{
    QMutexLocker lock(&mutex);
    return uri;
}

glaxnimate::android::AndroidIntentHandler::AndroidIntentHandler()
{

}


