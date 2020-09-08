#include "lottie_format.hpp"
#include "cbor_write_json.hpp"

#include <zlib.h>

namespace io::lottie {


class TgsFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Telegram Animated Sticker"); }
    QStringList extensions() const override { return {"tgs"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }

private:
#define CALL_ZLIB(func, ...) \
    zlib_check(#func, func(__VA_ARGS__))

    bool zlib_check(const char* func, int result)
    {
        if ( result == Z_OK || result == Z_STREAM_END )
            return true;

        error(tr("ZLib %1 returned %2").arg(func).arg(result));
        return false;
    }

    bool on_save(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override
    {
        QCborMap json = LottieFormat::to_json(document, true);
        json[QLatin1String("tgs")] = 1;
        QByteArray data = cbor_write_json(json, true);
        z_stream zip_stream;

        zip_stream.zalloc = Z_NULL;
        zip_stream.zfree  = Z_NULL;
        zip_stream.opaque = Z_NULL;
        if ( !CALL_ZLIB(deflateInit2, &zip_stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) )
            return false;

        const int chunk_size = 0x4000;
        Bytef buffer[chunk_size];
        zip_stream.next_in = (Bytef*) data.data();
        zip_stream.avail_in = data.size();
        zip_stream.avail_out = 0;

        while ( zip_stream.avail_out == 0 )
        {
            zip_stream.avail_out = chunk_size;
            zip_stream.next_out = buffer;
            CALL_ZLIB(deflate, &zip_stream, Z_FINISH);
            file.write((const char*)buffer, chunk_size - zip_stream.avail_out);
        }

        return CALL_ZLIB(deflateEnd, &zip_stream);
    }


    static Autoreg<TgsFormat> autoreg;
};

Autoreg<TgsFormat> TgsFormat::autoreg = {};

} // namespace io::lottie

#include "tgs_format.moc"
