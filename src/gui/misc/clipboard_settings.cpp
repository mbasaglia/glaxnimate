#include "clipboard_settings.hpp"

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSpacerItem>

#include "app/application.hpp"
#include "io/io_registry.hpp"

static std::vector<ClipboardSettings::MimeSettings>& mutable_mime_types()
{
    static std::vector<ClipboardSettings::MimeSettings> settings {
        {io::IoRegistry::instance().serializer_from_slug("glaxnimate"), true, QIcon(app::Application::instance()->data_file("images/logo.svg"))},
        {io::IoRegistry::instance().serializer_from_slug("svg"),        true, QIcon::fromTheme("image-svg+xml")},
        {io::IoRegistry::instance().serializer_from_slug("raster"),     false, QIcon::fromTheme("image-png")},
        {io::IoRegistry::instance().serializer_from_slug("json"),       false, QIcon::fromTheme("application-json")},
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
        if ( set.serializer->slug() != "glaxnimate" )
            set.enabled = settings.value(set.serializer->slug(), set.enabled).toBool();
}

void ClipboardSettings::save(QSettings & settings)
{
    for ( auto& set : mutable_mime_types() )
        if ( set.serializer->slug() != "glaxnimate" )
            settings.setValue(set.serializer->slug(), set.enabled);
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
        if ( mt.serializer->slug() == "glaxnimate" )
            check->setEnabled(false);
        else
            QObject::connect(check, &QCheckBox::clicked, [&mt](bool b){ mt.enabled = b; });
        lay->addWidget(check);
    }

    lay->insertStretch(-1);

    return wid;
}


