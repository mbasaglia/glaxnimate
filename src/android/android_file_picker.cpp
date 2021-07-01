#include "android_file_picker.hpp"

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
        static constexpr int RequestOpen = 123;
        static constexpr int RequestSave = 456;

        ResultReceiver(AndroidFilePicker *parent)
            : parent(parent)
        {}

        void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data)
        {
            if (receiverRequestCode == RequestOpen )
            {
                emit parent->open_selected(result_to_url(resultCode, data));
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

//    static jstring string(const QString& str)
//    {
//        return QAndroidJniObject::fromString(str).object<jstring>();
//    }

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

    QByteArray read_content_uri(const QString &cppuri)
    {
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
            "android/net/Uri",
            "parse",
            "(Ljava/lang/String;)Landroid/net/Uri;",
            QAndroidJniObject::fromString(cppuri).object<jstring>()
        );

        QAndroidJniObject intent("android/content/Intent");
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
