#pragma once


#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace io::video {

class VideoFormat : public ImportExport
{
    Q_OBJECT

public:
    QString name() const override { return tr("Video"); }
    QStringList extensions() const override;
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }
    io::SettingList save_settings() const override;

protected:
    bool on_save(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<VideoFormat> autoreg;
};

} // namespace io::video
