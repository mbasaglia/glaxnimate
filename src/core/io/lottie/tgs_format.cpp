#include "lottie_format.hpp"
#include "cbor_write_json.hpp"
#include "utils/gzip.hpp"

namespace io::lottie {


class TgsFormat : public LottieFormat
{
    Q_OBJECT

public:
    QString name() const override { return tr("Telegram Animated Sticker"); }
    QStringList extensions() const override { return {"tgs"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    SettingList save_settings() const override { return {}; }

private:
    bool on_save(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override
    {
        qreal height = document->main_composition()->height.get();
        if ( height != 512 )
            error(tr("Invalid height: %1, should be 512").arg(height));

        qreal width = document->main_composition()->height.get();
        if ( width != 512 )
            error(tr("Invalid width: %1, should be 512").arg(width));

        qreal fps = document->main_composition()->fps.get();
        if ( fps != 30 && fps != 60 )
            error(tr("Invalid fps: %1, should be 30 or 60").arg(fps));

        QCborMap json = LottieFormat::to_json(document, true);
        json[QLatin1String("tgs")] = 1;
        QByteArray data = cbor_write_json(json, true);

        quint32 compressed_size = 0;
        if ( !utils::gzip::compress(data, file, [this](const QString& s){ error(s); }, 9, &compressed_size) )
            return false;

        qreal size_k = compressed_size / 1024.0;
        if ( size_k > 64 )
            error(tr("File too large: %1k, should be under 64k").arg(size_k));

        return true;
    }

    bool on_open(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override
    {
        QByteArray json;
        if ( !utils::gzip::decompress(file, json, [this](const QString& s){ error(s); }) )
            return false;
        return load_json(json, document);
    }


    static Autoreg<TgsFormat> autoreg;
};

Autoreg<TgsFormat> TgsFormat::autoreg = {};

} // namespace io::lottie

#include "tgs_format.moc"
