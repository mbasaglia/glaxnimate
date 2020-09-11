#include "clipboard_settings.hpp"

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSpacerItem>

#include "app/application.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"
#include "io/mime/txt_mime.hpp"
#include "io/mime/svg_mime.hpp"
#include "io/mime/raster_mime.hpp"

static std::vector<ClipboardSettings::MimeSettings>& mutable_mime_types()
{
    static std::vector<ClipboardSettings::MimeSettings> settings {
        {"", io::glaxnimate::GlaxnimateFormat::instance(), true, QIcon(app::Application::instance()->data_file("data/images/logo/svg"))},
        {"svg", new io::mime::SvgMime, false, QIcon::fromTheme("image-svg+xml")},
        {"raster", new io::mime::RasterMime, false, QIcon::fromTheme("image-png")},
        {"json", new io::mime::JsonMime, false, QIcon::fromTheme("application-json")},
    };
    return settings;
}

const std::vector<ClipboardSettings::MimeSettings>& ClipboardSettings::mime_types()
{
    return mutable_mime_types();
}

void ClipboardSettings::load(const QSettings & settings)
{
    for ( auto& set : mutable_mime_types() )
        if ( !set.slug.isEmpty() )
            set.enabled = settings.value(set.slug, set.enabled).toBool();
}

void ClipboardSettings::save(QSettings & settings)
{
    for ( auto& set : mutable_mime_types() )
        if ( !set.slug.isEmpty() )
            settings.setValue(set.slug, set.enabled);
}

QWidget * ClipboardSettings::make_widget(QWidget* parent)
{
    QWidget* wid = new QWidget(parent);
    QVBoxLayout* lay = new QVBoxLayout(wid);
    wid->setLayout(lay);

    for ( auto& mt : mutable_mime_types() )
    {
        QCheckBox* check = new QCheckBox(mt.serializer->name(), parent);
        check->setCheckable(true);
        check->setChecked(mt.enabled);
        check->setIcon(mt.icon);
        if ( mt.slug.isEmpty() )
            check->setEnabled(false);
        else
            QObject::connect(check, &QCheckBox::clicked, [&mt](bool b){ mt.enabled = b; });
        lay->addWidget(check);
    }

    lay->insertStretch(-1);

    return wid;
}


