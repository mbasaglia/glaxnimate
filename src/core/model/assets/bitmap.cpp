#include "bitmap.hpp"
#include <QPainter>
#include <QImageWriter>
#include <QImageReader>
#include <QFileInfo>
#include <QBuffer>
#include <QUrl>

#include "model/document.hpp"
#include "model/assets/assets.hpp"
#include "command/object_list_commands.hpp"

GLAXNIMATE_OBJECT_IMPL(model::Bitmap)
GLAXNIMATE_OBJECT_IMPL(model::BitmapSequence)

void model::BitmapBase::paint(QPainter* painter, FrameTime time) const
{
    painter->drawPixmap(0, 0, pixmap(time));
}

void model::Bitmap::refresh(bool rebuild_embedded)
{
    QImageReader reader;
    QImage qimage;

    if ( (rebuild_embedded && !filename.get().isEmpty()) || data.get().isEmpty() )
    {
        QFileInfo finfo = file_info();
        if ( !finfo.isFile() )
            return;
        reader.setFileName(finfo.absoluteFilePath());
        format.set(reader.format());
        qimage = reader.read();
        if ( rebuild_embedded && embedded() )
            data.set(build_embedded(qimage));
    }
    else
    {
        QBuffer buf(const_cast<QByteArray*>(&data.get()));
        buf.open(QIODevice::ReadOnly);
        reader.setDevice(&buf);
        format.set(reader.format());
        qimage = reader.read();
    }

    image_ = QPixmap::fromImage(qimage);
    width.set(image_.width());
    height.set(image_.height());

    emit loaded();
}

QByteArray model::Bitmap::build_embedded(const QImage& img)
{
    QByteArray new_data;
    QBuffer buf(&new_data);
    buf.open(QIODevice::WriteOnly);
    QImageWriter writer(&buf, format.get().toLatin1());
    writer.write(img);
    return new_data;
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
        data.set_undoable({});
    else
        data.set_undoable(build_embedded(image_.toImage()));
}

void model::Bitmap::on_refresh()
{
    refresh(false);
}

QIcon model::Bitmap::instance_icon() const
{
    return image_;
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
    filename.set(file);
    return !image_.isNull();
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
    return !image_.isNull();
}

QUrl model::Bitmap::to_url() const
{
    if ( !embedded() )
    {
        return QUrl::fromLocalFile(file_info().absoluteFilePath());
    }

    QByteArray fmt = format.get().toLatin1();
    QByteArray mime_type;
    for ( const auto& mime : QImageWriter::supportedMimeTypes() )
        if ( QImageWriter::imageFormatsForMimeType(mime).contains(fmt) )
        {
            mime_type = mime;
            break;
        }

    if ( mime_type.isEmpty() )
        return {};

    QString data_url = "data:";
    data_url += mime_type;
    data_url += ";base64,";
    data_url += data.get().toBase64();
    return QUrl(data_url);
}

QString model::Bitmap::object_name() const
{
    if ( embedded() )
        return tr("Embedded image");
    return QFileInfo(filename.get()).fileName();
}


QFileInfo model::Bitmap::file_info() const
{
    return QFileInfo(document()->io_options().path, filename.get());
}


bool model::BitmapBase::remove_if_unused(bool)
{
    if ( users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->images->values
        ));
        return true;
    }
    return false;
}

void model::Bitmap::set_pixmap(const QImage& pix, const QString& format)
{
    this->format.set(format);
    data.set(build_embedded(pix));
}

int model::BitmapSequence::frame_number(model::FrameTime time) const
{
    if ( frames.empty() )
        return -1;

    float doc_fps = document()->main()->fps.get();

    if ( fps.get() <= 0 || doc_fps <= 0 )
        return 0;

    int frame = qRound(time / doc_fps * fps.get());
    if ( frame < 0 )
    {
        switch ( end_mode.get() )
        {
            case End:
                return -1;
            case Hold:
                return 0;
            case Loop:
                // I hate how % works for negative numbers...
                return ((frame % frames.size()) + frames.size()) % frames.size();
        }
    }

    if ( frame >= frames.size() )
    {
        switch ( end_mode.get() )
        {
            case End:
                return -1;
            case Hold:
                return frames.size() - 1;
            case Loop:
                return frame % frames.size();
        }
    }

    return frame;
}


model::Bitmap * model::BitmapSequence::frame(model::FrameTime time) const
{
    int frame = frame_number(time);
    if ( frame == -1 )
        return nullptr;
    return frames[frame];
}

model::Bitmap * model::BitmapSequence::add_frame()
{
    std::unique_ptr<model::Bitmap> bmp = std::make_unique<model::Bitmap>(document());
    return frames.insert(std::move(bmp));
}

void model::BitmapSequence::add_frame(std::unique_ptr<Bitmap> frame)
{
    frames.insert(std::move(frame));
}

QPixmap model::BitmapSequence::pixmap(model::FrameTime time) const
{
    if ( auto bitmap = frame(time) )
        return bitmap->pixmap();
    return {};
}

QImage model::BitmapSequence::image(model::FrameTime time) const
{
    if ( auto bitmap = frame(time) )
        return bitmap->image();
    return {};
}

QIcon model::BitmapSequence::instance_icon() const
{
    if ( frames.empty() )
        return {};
    return frames[0]->pixmap();
}
