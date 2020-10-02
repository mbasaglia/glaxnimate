#include "bitmap.hpp"
#include <QPainter>
#include <QImageWriter>
#include <QImageReader>
#include <QFileInfo>
#include <QBuffer>

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
