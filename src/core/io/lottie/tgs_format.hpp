#pragma once
#include "lottie_format.hpp"

namespace io::lottie {


class TgsFormat : public LottieFormat
{
    Q_OBJECT

public:
    QString slug() const override { return "tgs"; }
    QString name() const override { return tr("Telegram Animated Sticker"); }
    QStringList extensions() const override { return {"tgs"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    SettingList save_settings() const override { return {}; }

    void validate(model::Document* document);

private:
    bool on_save(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override;

    bool on_open(QIODevice& file, const QString&,
                 model::Document* document, const QVariantMap&) override;


    static Autoreg<TgsFormat> autoreg;
};

} // namespace io::lottie
