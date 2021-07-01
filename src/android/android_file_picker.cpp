#include "android_file_picker.hpp"

#include <QAndroidJniObject>
#include <QtAndroid>
#include <QAndroidActivityResultReceiver>
#include <QAndroidJniEnvironment>
#include <QDebug>
class glaxnimate::android::AndroidFilePicker::Private
{
public:
    class ResultReceiver : public QAndroidActivityResultReceiver
    {

    public:
        static constexpr int RequestOpen = 1;
        static constexpr int RequestSave = 2;
        static constexpr int RequestExport = 3;

        ResultReceiver(AndroidFilePicker *parent)
            : parent(parent)
        {}

        void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data)
        {
            qDebug() << "handle result" << receiverRequestCode << resultCode;
            switch ( receiverRequestCode )
            {
                case RequestOpen:
                    emit parent->open_selected(result_to_url(resultCode, data));
                    break;
                case RequestSave:
                    emit parent->save_selected(result_to_url(resultCode, data), false);
                    break;
                case RequestExport:
                    emit parent->save_selected(result_to_url(resultCode, data), true);
                    break;
            }
        }

    private:
        QUrl result_to_url(int resultCode, const QAndroidJniObject &data)
        {
            jint RESULT_OK = QAndroidJniObject::getStaticField<jint>("android/app/Activity", "RESULT_OK");
            if ( resultCode != RESULT_OK )
                return {};

            auto uri = data.callObjectMethod("getData", "()Landroid/net/Uri;");
            return uri.toString();
        }

        AndroidFilePicker *parent;
    };

    Private(AndroidFilePicker* parent)
        : receiver(parent)
    {}

    bool select_open()
    {
        QAndroidJniObject ACTION_OPEN_DOCUMENT = QAndroidJniObject::fromString("android.intent.action.OPEN_DOCUMENT");
        QAndroidJniObject intent("android/content/Intent");
        if ( !ACTION_OPEN_DOCUMENT.isValid() || !intent.isValid())
            return false;

        QAndroidJniObject CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
        intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object<jstring>());

        intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", ACTION_OPEN_DOCUMENT.object<jstring>());
        intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("*/*").object<jstring>());
        QtAndroid::startActivity(intent.object<jobject>(), ResultReceiver::RequestOpen, &receiver);

        return true;
    }


    bool select_save(const QString &suggested_name, bool is_export)
    {
        QAndroidJniObject ACTION_SAVE_DOCUMENT = QAndroidJniObject::fromString("android.intent.action.CREATE_DOCUMENT");
        QAndroidJniObject intent("android/content/Intent");
        if ( !ACTION_SAVE_DOCUMENT.isValid() || !intent.isValid())
            return false;

        QAndroidJniObject CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
        intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object<jstring>());

        intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", ACTION_SAVE_DOCUMENT.object<jstring>());
        intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("*/*").object<jstring>());

        if ( !suggested_name.isEmpty() )
        {
            auto title = QAndroidJniObject::fromString("android.intent.extra.TITLE");
            auto j_suggested_name = QAndroidJniObject::fromString(suggested_name);
            intent.callObjectMethod(
                "putExtra",
                "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                title.object<jstring>(),
                j_suggested_name.object<jstring>()
            );
        }

        QtAndroid::startActivity(intent.object<jobject>(), is_export ? ResultReceiver::RequestExport : ResultReceiver::RequestSave, &receiver);

        return true;
    }

    QByteArray read_content_uri(const QString &cppuri)
    {
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
            "android/net/Uri",
            "parse",
            "(Ljava/lang/String;)Landroid/net/Uri;",
            QAndroidJniObject::fromString(cppuri).object<jstring>()
        );

        QAndroidJniObject contentResolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        QAndroidJniObject input_stream = contentResolver.callObjectMethod(
            "openInputStream",
            "(Landroid/net/Uri;)Ljava/io/InputStream;",
            uri.object<jobject>()
        );

        QAndroidJniEnvironment env;
        jint avail = input_stream.callMethod<jint>("available", "()I");
        int chunk_size = 1024 * 10;
        if ( chunk_size > avail )
            chunk_size = avail;
        jbyteArray jdata = env->NewByteArray(chunk_size);
        QByteArray qdata;
        while ( true )
        {
            jint read_amount = input_stream.callMethod<jint>("read", "([B)I", jdata);
            if ( read_amount == -1 )
                break;
            jbyte* chunk = env->GetByteArrayElements(jdata, nullptr);
            qdata.append((char*)chunk, read_amount);
        }
        env->DeleteLocalRef(jdata);

        qDebug() << "read:" << qdata;
        return qdata;
    }

    bool write_content_uri(const QString& cppuri, const QByteArray& data)
    {
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
            "android/net/Uri",
            "parse",
            "(Ljava/lang/String;)Landroid/net/Uri;",
            QAndroidJniObject::fromString(cppuri).object<jstring>()
        );

        if ( !uri.isValid() )
            return false;

        QAndroidJniObject contentResolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        if ( !contentResolver.isValid() )
            return false;

        QAndroidJniObject output = contentResolver.callObjectMethod(
            "openOutputStream",
            "(Landroid/net/Uri;)Ljava/io/OutputStream;",
            uri.object<jobject>()
        );


        if ( !output.isValid() )
            return false;


        QAndroidJniEnvironment env;
        int chunk_size = 1024 * 10;
        if ( data.size() < chunk_size )
            chunk_size = data.size();
        jbyteArray jdata = env->NewByteArray(chunk_size);
        for ( int i = 0; i < data.size(); i+= chunk_size )
        {
            if ( i + chunk_size >= data.size() )
                chunk_size = data.size() - i;
            env->SetByteArrayRegion(jdata, 0, chunk_size, (jbyte*)data.data());
            env->SetByteArrayRegion(jdata, 0, chunk_size, (jbyte*)data.data());
            env->SetByteArrayRegion(jdata, 0, chunk_size, (jbyte*)data.data());
            output.callMethod<void>("write", "([BII)V", jdata, jint(0), jint(chunk_size));
        }
        env->DeleteLocalRef(jdata);

        output.callMethod<void>("close", "()V");

        qDebug() << "written:" << data;

        return true;
   }

    ResultReceiver receiver;
};


glaxnimate::android::AndroidFilePicker::AndroidFilePicker(QObject *parent)
    : QObject(parent), d(std::make_unique<Private>(this))
{
}

glaxnimate::android::AndroidFilePicker::~AndroidFilePicker() = default;

bool glaxnimate::android::AndroidFilePicker::select_open()
{
    get_permissions();
    return d->select_open();
}

QByteArray glaxnimate::android::AndroidFilePicker::read_content_uri(const QUrl &url)
{
    return d->read_content_uri(url.toString());
}

bool glaxnimate::android::AndroidFilePicker::write_content_uri(const QUrl &url, const QByteArray &data)
{
    return d->write_content_uri(url.toString(), data);
}

bool glaxnimate::android::AndroidFilePicker::get_permissions(const QStringList& permissions)
{
    for ( const QString &permission : permissions )
    {
        auto result = QtAndroid::checkPermission(permission);
        if ( result == QtAndroid::PermissionResult::Denied )
        {
            auto resultHash = QtAndroid::requestPermissionsSync(QStringList({permission}));
            if ( resultHash[permission] == QtAndroid::PermissionResult::Denied )
                return false;
        }
    }

    return true;
}

bool glaxnimate::android::AndroidFilePicker::select_save(const QString &suggested_name, bool is_export)
{
    get_permissions();
    return d->select_save(suggested_name, is_export);
}
