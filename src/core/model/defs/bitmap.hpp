#pragma once

#include <QPixmap>
#include <QFileInfo>
#include <QUrl>

#include "model/defs/asset.hpp"

namespace model {

class Bitmap : public Asset
{
    GLAXNIMATE_OBJECT(Bitmap)
    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY(QString, filename, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY_RO(QString, format, {})
    GLAXNIMATE_PROPERTY_RO(int, width, -1)
    GLAXNIMATE_PROPERTY_RO(int, height, -1)
    Q_PROPERTY(bool embedded READ embedded WRITE embed)

public:
    using Asset::Asset;

    void paint(QPainter* painter) const;

    bool embedded() const;

    QIcon reftarget_icon() const override;

    QString type_name_human() const override
    {
        return tr("Bitmap");
    }

    bool from_url(const QUrl& url);
    bool from_file(const QString& file);
    bool from_base64(const QString& data);

    QUrl to_url() const;

    QString object_name() const override;

    QFileInfo file_info() const;

    const QPixmap& pixmap() const { return image; }
    void set_pixmap(const QImage& qimage, const QString& format);

    bool remove_if_unused(bool clean_lists) override;

public slots:
    void refresh(bool rebuild_embedded);

    void embed(bool embedded);

private:
    QByteArray build_embedded(const QImage& img);

private slots:
    void on_refresh();

signals:
    void loaded();

private:
    QPixmap image;

};

} // namespace model
