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
        QCborMap json = LottieFormat::to_json(document, true);
        json[QLatin1String("tgs")] = 1;
        QByteArray data = cbor_write_json(json, true);
        return utils::gzip::compress(data, file, [this](const QString& s){ error(s); });
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
