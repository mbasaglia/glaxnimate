#pragma once
#include "app/settings/custom_settings_group.hpp"
#include "io/mime/mime_serializer.hpp"

class ClipboardSettings : public app::settings::CustomSettingsGroupBase
{
public:
    QString slug() const override { return "clipboard"; }
    QString label() const override { return QObject::tr("Clipboard"); }
    QIcon icon() const override { return QIcon::fromTheme("edit-paste"); }
    void load(const QSettings & settings) override;
    void save(QSettings & settings) override;
    QWidget * make_widget(QWidget * parent) override;

    struct MimeSettings
    {
        io::mime::MimeSerializer* serializer;
        bool enabled;
        QIcon icon;
    };

    static const std::vector<MimeSettings>& mime_types();

};
