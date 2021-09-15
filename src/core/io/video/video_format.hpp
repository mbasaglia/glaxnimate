#pragma once


#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::video {

class VideoFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "video"; }
    QString name() const override { return tr("Video"); }
    QStringList extensions() const override;
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }
    std::unique_ptr<app::settings::SettingsGroup> save_settings(model::Document*) const override;

    static QString library_version();

protected:
    bool on_save(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<VideoFormat> autoreg;
};

} // namespace glaxnimate::io::video
