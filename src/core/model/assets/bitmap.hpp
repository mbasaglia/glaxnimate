#pragma once

#include <QPixmap>
#include <QImage>
#include <QFileInfo>
#include <QUrl>

#include "model/assets/asset.hpp"
#include "model/property/object_list_property.hpp"

namespace model {


class BitmapBase : public Asset
{
    Q_OBJECT
    GLAXNIMATE_PROPERTY_RO(QString, format, {})
    GLAXNIMATE_PROPERTY_RO(int, width, -1)
    GLAXNIMATE_PROPERTY_RO(int, height, -1)

public:
    using Asset::Asset;

    bool remove_if_unused(bool clean_lists) override;

    void paint(QPainter* painter, FrameTime time) const;

    Q_INVOKABLE virtual QPixmap pixmap(FrameTime time) const = 0;
    Q_INVOKABLE virtual QImage image(FrameTime time) const = 0;

};

class Bitmap : public BitmapBase
{
    GLAXNIMATE_OBJECT(Bitmap)
    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY(QString, filename, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY_RO(QString, format, {})
    Q_PROPERTY(bool embedded READ embedded WRITE embed)

public:
    using BitmapBase::BitmapBase;

    bool embedded() const;

    QIcon instance_icon() const override;

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

    void set_pixmap(const QImage& qimage, const QString& format);

    const QPixmap& pixmap() const { return image_; }
    QImage image() const
    {
        return image_.toImage();
    }

    QPixmap pixmap(FrameTime) const override
    {
        return pixmap();
    }

    QImage image(FrameTime) const override
    {
        return image();
    }

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
    QPixmap image_;
};



class BitmapSequence : public BitmapBase
{
    GLAXNIMATE_OBJECT(BitmapSequence)
    GLAXNIMATE_PROPERTY(float, fps, 24)
    GLAXNIMATE_PROPERTY_LIST(Bitmap, frames)

public:
    enum EndMode
    {
        End = 0,
        Loop = 1,
        Hold = 2,
    };

    Q_ENUM(EndMode)
    GLAXNIMATE_PROPERTY(EndMode, end_mode, End)

public:
    using BitmapBase::BitmapBase;

private:
    QPixmap pixmap(FrameTime time) const override;
    QImage image(FrameTime time) const override;
    Q_INVOKABLE Bitmap* frame(FrameTime time) const;
    Q_INVOKABLE int frame_number(FrameTime time) const;

    QIcon instance_icon() const override;

    QString type_name_human() const override
    {
        return tr("Bitmap Sequence");
    }

    Q_INVOKABLE Bitmap* add_frame();
    void add_frame(std::unique_ptr<Bitmap> frame);
};


} // namespace model
