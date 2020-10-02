#include "bitmap.hpp"
#include <QPainter>
#include <QImageWriter>
#include <QImageReader>
#include <QFileInfo>
#include <QBuffer>
#include <QUrl>

GLAXNIMATE_OBJECT_IMPL(model::Bitmap)

void model::Bitmap::paint(QPainter* painter) const
{
    painter->drawPixmap(0, 0, image);
}

void model::Bitmap::refresh(bool rebuild_embedded)
{
    QImageReader reader;
    QImage qimage;

    if ( rebuild_embedded || data.get().isEmpty() )
    {
        if ( !QFileInfo::exists(filename.get()) )
            return;
        reader.setFileName(filename.get());
        format.set(reader.format());
        qimage = reader.read();
        if ( rebuild_embedded && embedded() )
            build_embedded(qimage);
    }
    else
    {
        QBuffer buf(const_cast<QByteArray*>(&data.get()));
        buf.open(QIODevice::ReadOnly);
        reader.setDevice(&buf);
        format.set(reader.format());
        qimage = reader.read();
    }

    image = QPixmap::fromImage(qimage);
    width.set(image.width());
    height.set(image.height());

    emit loaded();
}

void model::Bitmap::build_embedded(const QImage& img)
{
    QByteArray new_data;
    QBuffer buf(&new_data);
    buf.open(QIODevice::WriteOnly);
    QImageWriter writer(&buf, format.get().toLatin1());
    writer.write(img);
    data.set(new_data);
}

bool model::Bitmap::embedded() const
{
    return !data.get().isEmpty();
}

void model::Bitmap::embed(bool embedded)
{
    if ( embedded == this->embedded() )
        return;

    if ( !embedded )
        data.set({});
    else
        build_embedded(image.toImage());
}

void model::Bitmap::on_refresh()
{
    refresh(false);
}

QIcon model::Bitmap::reftarget_icon() const
{
    return image;
}

bool model::Bitmap::from_url(const QUrl& url)
{
    if ( url.scheme().isEmpty() || url.scheme() == "file" )
        return from_file(url.path());

    if ( url.scheme() == "data" )
        return from_base64(url.path());

    return false;
}

bool model::Bitmap::from_file(const QString& file)
{
    if ( !QFileInfo(file).isFile() )
        return false;

    filename.set(file);
    return !image.isNull();
}

bool model::Bitmap::from_base64(const QString& data)
{
    auto chunks = data.splitRef(',');
    if ( chunks.size() != 2 )
        return false;
    auto mime_settings = chunks[0].split(';');
    if ( mime_settings.size() != 2 || mime_settings[1] != "base64" )
        return false;

    auto formats = QImageReader::imageFormatsForMimeType(mime_settings[0].toLatin1());
    if ( formats.empty() )
        return false;

    auto decoded = QByteArray::fromBase64(chunks[1].toLatin1());
    format.set(formats[0]);
    this->data.set(decoded);
    return !image.isNull();
}
