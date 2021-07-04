#ifndef GLAXNIMATE_ANDROID_ANDROIDFILEPICKER_HPP
#define GLAXNIMATE_ANDROID_ANDROIDFILEPICKER_HPP

#include <memory>
#include <QUrl>
#include <QObject>

namespace glaxnimate::android {

class AndroidFilePicker : public QObject
{
    Q_OBJECT
public:
    AndroidFilePicker(QObject *parent = nullptr);
    ~AndroidFilePicker();

    bool select_open(bool is_import);
    bool select_save(const QString& suggested_name, bool is_export, const QString &mime = "*/*");
    bool open_external(const QUrl& uri, const QString& mime);

    static QByteArray read_content_uri(const QUrl& url);
    static bool write_content_uri(const QUrl& url, const QByteArray& data);

    bool get_permissions(const QStringList & perms = {
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.READ_EXTERNAL_STORAGE"
    });

signals:
    void open_selected(const QUrl& path, bool is_import);
    void save_selected(const QUrl& path, bool is_export);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_ANDROIDFILEPICKER_HPP
