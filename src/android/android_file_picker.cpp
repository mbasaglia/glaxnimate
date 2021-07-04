#include "android_file_picker.hpp"

#include <QtGlobal>

#ifdef Q_OS_ANDROID_FAKE

class glaxnimate::android::AndroidFilePicker::Private
{
public:
    Private(AndroidFilePicker*)
    {}

    bool select_open(bool)
    {
        return false;
    }


    bool select_save(const QString &, bool, const QString&)
    {
        return false;
    }

    static QByteArray read_content_uri(const QString &)
    {
        return {};
    }

    static bool write_content_uri(const QString&, const QByteArray&)
    {
        return false;
    }

    static bool get_permissions(const QStringList&)
    {
        return false;
    }

    bool open_external(const QUrl &, const QString &)
    {
        return false;
    }
};
#else

#include <QAndroidJniObject>
#include <QtAndroid>
#include <QAndroidActivityResultReceiver>
#include <QAndroidJniEnvironment>

class glaxnimate::android::AndroidFilePicker::Private
{
public:
    class ResultReceiver : public QAndroidActivityResultReceiver
    {

    public:
        enum
        {
            RequestOpen,
            RequestSave,
            RequestExport,
            RequestView,
            RequestImport,
        };

        ResultReceiver(AndroidFilePicker *parent)
            : parent(parent)
        {}

        void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data)
        {
            switch ( receiverRequestCode )
            {
                case RequestOpen:
                    emit parent->open_selected(result_to_url(resultCode, data), false);
                    break;
                case RequestImport:
                    emit parent->open_selected(result_to_url(resultCode, data), true);
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

    bool select_open(bool is_import)
    {
        QAndroidJniObject ACTION_OPEN_DOCUMENT = QAndroidJniObject::fromString("android.intent.action.OPEN_DOCUMENT");
        QAndroidJniObject intent("android/content/Intent");
        if ( !ACTION_OPEN_DOCUMENT.isValid() || !intent.isValid())
            return false;

        QAndroidJniObject CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
        intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object<jstring>());

        intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", ACTION_OPEN_DOCUMENT.object<jstring>());
        intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("*/*").object<jstring>());
        QtAndroid::startActivity(intent.object<jobject>(), is_import ? ResultReceiver::RequestImport : ResultReceiver::RequestOpen , &receiver);

        return true;
    }


    bool select_save(const QString &suggested_name, bool is_export, const QString& mime)
    {
        QAndroidJniObject ACTION_SAVE_DOCUMENT = QAndroidJniObject::fromString("android.intent.action.CREATE_DOCUMENT");
        QAndroidJniObject intent("android/content/Intent");
        if ( !ACTION_SAVE_DOCUMENT.isValid() || !intent.isValid())
            return false;

        QAndroidJniObject CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField("android/content/Intent", "CATEGORY_OPENABLE", "Ljava/lang/String;");
        intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object<jstring>());

        intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", ACTION_SAVE_DOCUMENT.object<jstring>());
        intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString(mime).object<jstring>());

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

    static QByteArray read_content_uri(const QString &cppuri)
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

        return qdata;
    }

    static bool write_content_uri(const QString& cppuri, const QByteArray& data)
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
        jbyteArray jdata = env->NewByteArray(chunk_size);

        int written;
        for ( written = 0; written < data.size(); written+= chunk_size )
        {
            if ( written + chunk_size >= data.size() )
                chunk_size = data.size() - written;
            env->SetByteArrayRegion(jdata, 0, chunk_size, (jbyte*)data.data() + written);
            output.callMethod<void>("write", "([BII)V", jdata, jint(0), jint(chunk_size));
        }

        env->DeleteLocalRef(jdata);

        output.callMethod<void>("close", "()V");

        return true;
   }

    static bool get_permissions(const QStringList& permissions)
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


    bool open_external(const QUrl &cppuri, const QString &mime)
    {

        QAndroidJniObject ACTION_VIEW = QAndroidJniObject::fromString("android.intent.action.VIEW");
        QAndroidJniObject intent("android/content/Intent");
        if ( !ACTION_VIEW.isValid() || !intent.isValid())
            return false;

        auto uri_str = QAndroidJniObject::fromString(cppuri.toString());
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
            "android/net/Uri",
            "parse",
            "(Ljava/lang/String;)Landroid/net/Uri;",
            uri_str.object<jstring>()
        );
        auto jmime = QAndroidJniObject::fromString(mime);
        intent.callObjectMethod(
            "setDataAndType",
            "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;",
            uri_str.object<jobject>(),
            jmime.object<jstring>()
        );
        QtAndroid::startActivity(intent.object<jobject>(), ResultReceiver::RequestView, &receiver);


        return true;
    }


    ResultReceiver receiver;
};
#endif


glaxnimate::android::AndroidFilePicker::AndroidFilePicker(QObject *parent)
    : QObject(parent), d(std::make_unique<Private>(this))
{
}

glaxnimate::android::AndroidFilePicker::~AndroidFilePicker() = default;

bool glaxnimate::android::AndroidFilePicker::select_open(bool is_import)
{
    get_permissions();
    return d->select_open(is_import);
}

QByteArray glaxnimate::android::AndroidFilePicker::read_content_uri(const QUrl &url)
{
    return Private::read_content_uri(url.toString());
}

bool glaxnimate::android::AndroidFilePicker::write_content_uri(const QUrl &url, const QByteArray &data)
{
    return Private::write_content_uri(url.toString(), data);
}

bool glaxnimate::android::AndroidFilePicker::get_permissions(const QStringList& permissions)
{

    return Private::get_permissions(permissions);
}

bool glaxnimate::android::AndroidFilePicker::select_save(const QString &suggested_name, bool is_export, const QString& mime)
{
    get_permissions();
    return d->select_save(suggested_name, is_export, mime);
}

bool glaxnimate::android::AndroidFilePicker::open_external(const QUrl &cppuri, const QString &mime)
{
    return d->open_external(cppuri, mime);
}
