#include "gzip.hpp"

#include <cstring>

#include <QApplication>

#include <zlib.h>

namespace {

class Gzipper
{
public:
    static const int chunk_size = 0x4000;
    using Buffer = std::array<Bytef, chunk_size>;

    struct BufferView
    {
        const char* data;
        std::size_t size;
    };

    explicit Gzipper(const utils::gzip::ErrorFunc& on_error)
        : on_error(on_error)
    {
        zip_stream.zalloc = Z_NULL;
        zip_stream.zfree  = Z_NULL;
        zip_stream.opaque = Z_NULL;
    }

    void add_data(const QByteArray& data)
    {
        add_data(data.data(), data.size());
    }

    void add_data(const char* data, std::size_t size)
    {
        zip_stream.next_in = (Bytef*) data;
        zip_stream.avail_in = size;
        zip_stream.avail_out = 0;
    }

    bool finished() const
    {
        return zip_stream.avail_out != 0;
    }

    bool inflate_init()
    {
        process_fn = &inflate;
        end_fn = &inflateEnd;
        op = "inflate";
        return zlib_check("inflateInit2", inflateInit2(&zip_stream, 16|MAX_WBITS));
    }

    BufferView process()
    {
        zip_stream.avail_out = chunk_size;
        zip_stream.next_out = buffer.data();
        zlib_check(op, process_fn(&zip_stream, Z_FINISH));
        return {(const char*)buffer.data(), chunk_size - zip_stream.avail_out};
    }

    bool end()
    {
        return zlib_check(op, end_fn(&zip_stream), "End");
    }

    bool deflate_init(int level)
    {
        process_fn = &deflate;
        end_fn = &deflateEnd;
        op = "deflate";
        return zlib_check("deflateInit2", deflateInit2(&zip_stream, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY));
    }

    void log_error(const QString& msg)
    {
        on_error(msg);
    }

private:
    bool zlib_check(const char* func, int result, const char* extra = "")
    {
        if ( result >= 0 || result == Z_BUF_ERROR )
            return true;

        log_error(QApplication::tr("ZLib %1%2 returned %3").arg(func).arg(extra).arg(result));
        return false;
    }

    z_stream zip_stream;
    utils::gzip::ErrorFunc on_error;
    Buffer buffer;
    int (*process_fn)(z_streamp, int);
    int (*end_fn)(z_streamp);
    const char* op;
};

} // namespace

bool utils::gzip::compress(const QByteArray& data, QIODevice& output,
                           const utils::gzip::ErrorFunc& on_error, int level,
                           quint32* compressed_size)
{
    Gzipper gz(on_error);

    if ( !gz.deflate_init(level) )
        return false;

    gz.add_data(data);

    quint32 total_size = 0;
    while ( !gz.finished() )
    {
        auto bv = gz.process();
        output.write(bv.data, bv.size);
        total_size += bv.size;
    }

    if ( compressed_size )
        *compressed_size = total_size;

    return gz.end();
}

bool utils::gzip::decompress(QIODevice& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    Gzipper gz(on_error);
    if ( !gz.inflate_init() )
        return false;

    while ( true )
    {
        QByteArray data = input.read(Gzipper::chunk_size);
        if ( data.isEmpty() )
            break;

        gz.add_data(data);
        while ( !gz.finished() )
        {
            auto bv = gz.process();
            output.append(bv.data, bv.size);
        }
    }
    return gz.end();
}


bool utils::gzip::decompress(const QByteArray& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    Gzipper gz(on_error);
    if ( !gz.inflate_init() )
        return false;

    gz.add_data(input);

    while ( !gz.finished() )
    {
        auto bv = gz.process();
        output.append(bv.data, bv.size);
    }

    return gz.end();
}

bool utils::gzip::is_compressed(QIODevice& input)
{
    return input.peek(2) == "\x1f\x8b";
}

#include <QFile>
class utils::gzip::GzipStream::Private
{
public:
    Private(QIODevice* target, const ErrorFunc& ef)
    : zipper(ef), target(target)
    {}

    Gzipper zipper;
    QIODevice* target;
    QIODevice::OpenMode mode = QIODevice::NotOpen;
    qint64 total_size = 0;
    QByteArray buffer;

    QFile f{"/tmp/foo.txt"};
    void _memcpy(char* dest, const char* src, std::size_t size)
    {
        std::memcpy(dest, src, size);

        if ( !f.isOpen() )
            f.open(WriteOnly);
        f.write(src, size);
//         f.write("\n=====\n");
        f.flush();
    }
};


utils::gzip::GzipStream::GzipStream(QIODevice* target, const utils::gzip::ErrorFunc& on_error)
    : d(std::make_unique<Private>(target, on_error))
{}

utils::gzip::GzipStream::~GzipStream()
{
    if ( d->mode != NotOpen )
        d->zipper.end();
}

bool utils::gzip::GzipStream::open(QIODevice::OpenMode mode)
{
    if ( d->mode != NotOpen )
    {
        QString error = "Gzip stream already open";
        setErrorString(error);
        return false;
    }

    if ( mode == ReadOnly )
    {
        d->zipper.inflate_init();
        d->mode = ReadOnly;
        setOpenMode(d->mode);
        return true;
    }

    if ( mode == WriteOnly )
    {
        d->zipper.deflate_init(9);
        d->mode = WriteOnly;
        setOpenMode(d->mode);
        return true;
    }

    setErrorString("Unsupported open mode for Gzip stream");
    return false;
}

bool utils::gzip::GzipStream::atEnd() const
{
    return d->target->atEnd() && d->buffer.isEmpty();
}

qint64 utils::gzip::GzipStream::writeData(const char* data, qint64 len)
{
    if ( d->mode != WriteOnly )
    {
        setErrorString("Gzip stream not open for writing");
        return -1;
    }


    d->zipper.add_data(data, len);

    while ( !d->zipper.finished() )
    {
        auto bv = d->zipper.process();
        d->target->write(bv.data, bv.size);
        d->total_size += bv.size;
    }

    return len;
}

qint64 utils::gzip::GzipStream::readData(char* data, qint64 maxlen)
{
    if ( d->mode != ReadOnly )
    {
        setErrorString("Gzip stream not open for reading");
        return -1;
    }

    if ( maxlen <= 0 )
        return 0;

    qint64 read = 0;

    if ( !d->buffer.isEmpty() )
    {
        if ( d->buffer.size() < maxlen )
        {
            d->_memcpy(data, d->buffer.data(), d->buffer.size());
            maxlen -= d->buffer.size();
            data += d->buffer.size();
            read += d->buffer.size();
            d->buffer.clear();
        }
        else
        {
            d->_memcpy(data, d->buffer.data(), maxlen);
            d->buffer = d->buffer.mid(maxlen);
            return maxlen;
        }
    }

    while ( read < maxlen )
    {
        QByteArray buf = d->target->read(Gzipper::chunk_size);
        if ( buf.isEmpty() )
            break;

        d->zipper.add_data(buf);

        while ( !d->zipper.finished() )
        {
            auto bv = d->zipper.process();

            if ( qint64(read + bv.size) >= maxlen )
            {
                auto delta = maxlen - read;
                d->_memcpy(data + read, bv.data, delta);
                d->buffer = QByteArray(bv.data + delta, bv.size - delta);
                read = maxlen;

                while ( !d->zipper.finished() )
                {
                    bv = d->zipper.process();
                    d->buffer += QByteArray(bv.data, bv.size);
                }

                break;
            }
            else
            {
                d->_memcpy(data + read, bv.data, bv.size);
                read += bv.size;
            }
        }

    }

    d->total_size += read;
    return read;
}


qint64 utils::gzip::GzipStream::ouput_size() const
{
    return d->total_size;
}
