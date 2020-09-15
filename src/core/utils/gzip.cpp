#include "gzip.hpp"

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

private:
    bool zlib_check(const char* func, int result, const char* extra = "")
    {
        if ( result == Z_OK || result == Z_STREAM_END )
            return true;

        on_error(QApplication::tr("ZLib %1%2 returned %3").arg(func).arg(extra).arg(result));
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
                           const utils::gzip::ErrorFunc& on_error, int level)
{
    Gzipper gz(on_error);

    if ( !gz.deflate_init(level) )
        return false;

    gz.add_data(data);

    while ( !gz.finished() )
    {
        auto bv = gz.process();
        output.write(bv.data, bv.size);
    }

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
