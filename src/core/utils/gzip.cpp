#include "gzip.hpp"

#include <QApplication>
#include <zlib.h>


#define CALL_ZLIB(func, ...) \
    zlib_check(#func, func(__VA_ARGS__), on_error)

namespace {

const int chunk_size = 0x4000;
using Buffer = std::array<Bytef, chunk_size>;

bool zlib_check(const char* func, int result, const utils::gzip::ErrorFunc& on_error)
{
    if ( result == Z_OK || result == Z_STREAM_END )
        return true;

    on_error(QApplication::tr("ZLib %1 returned %2").arg(func).arg(result));
    return false;
}

struct Compressor
{
};

} // namespace

bool utils::gzip::compress(const QByteArray& data, QIODevice& output,
                           const utils::gzip::ErrorFunc& on_error, int level)
{
    z_stream zip_stream;

    zip_stream.zalloc = Z_NULL;
    zip_stream.zfree  = Z_NULL;
    zip_stream.opaque = Z_NULL;
    if ( !CALL_ZLIB(deflateInit2, &zip_stream, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) )
        return false;

    Buffer buffer;
    zip_stream.next_in = (Bytef*) data.data();
    zip_stream.avail_in = data.size();
    zip_stream.avail_out = 0;

    while ( zip_stream.avail_out == 0 )
    {
        zip_stream.avail_out = chunk_size;
        zip_stream.next_out = buffer.data();
        CALL_ZLIB(deflate, &zip_stream, Z_FINISH);
        output.write((const char*)buffer.data(), chunk_size - zip_stream.avail_out);
    }

    return CALL_ZLIB(deflateEnd, &zip_stream);
}

bool utils::gzip::decompress(QIODevice& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    return decompress(input.readAll(), output, on_error);
}


bool utils::gzip::decompress(const QByteArray& input, QByteArray& output, const utils::gzip::ErrorFunc& on_error)
{
    z_stream zip_stream;

    zip_stream.zalloc = Z_NULL;
    zip_stream.zfree  = Z_NULL;
    zip_stream.opaque = Z_NULL;
    if ( !CALL_ZLIB(inflateInit2, &zip_stream, 16|MAX_WBITS) )
        return false;

    Buffer buffer;
    zip_stream.next_in = (Bytef*) input.data();
    zip_stream.avail_in = input.size();
    zip_stream.avail_out = 0;

    while ( zip_stream.avail_out == 0 )
    {
        zip_stream.avail_out = chunk_size;
        zip_stream.next_out = buffer.data();
        CALL_ZLIB(inflate, &zip_stream, Z_FINISH);
        output.append((const char*)buffer.data(), chunk_size - zip_stream.avail_out);
    }


    return CALL_ZLIB(inflateEnd, &zip_stream);
}


