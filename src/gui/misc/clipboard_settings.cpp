#include "clipboard_settings.hpp"

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSpacerItem>

#include "app/application.hpp"

static bool svg = true;
static bool json = false;
static bool png = false;

void ClipboardSettings::load(const QSettings & settings)
{
    ::svg = settings.value("svg", ::svg).toBool();
    ::json = settings.value("json", ::json).toBool();
    ::png = settings.value("png", ::png).toBool();
}

void ClipboardSettings::save(QSettings & settings)
{
    settings.setValue("svg", ::svg);
    settings.setValue("json", ::json);
    settings.setValue("png", ::png);
}

static QCheckBox* mk_checkbox(const QString& name, const QString& icon, bool* target, QWidget* parent)
{
    QCheckBox* check = new QCheckBox(name, parent);
    check->setCheckable(true);
    check->setChecked(*target);
    check->setIcon(QIcon::fromTheme(icon));
    QObject::connect(check, &QCheckBox::clicked, [target](bool b){ *target = b; });
    return check;
}

QWidget * ClipboardSettings::make_widget(QWidget* parent)
{
    QWidget* wid = new QWidget(parent);
    QVBoxLayout* lay = new QVBoxLayout(wid);
    wid->setLayout(lay);


    QCheckBox* check = new QCheckBox(QWidget::tr("Rawr"), wid);
    check->setCheckable(true);
    check->setChecked(true);
    check->setEnabled(false);
    check->setIcon(QIcon(app::Application::instance()->data_file("data/images/logo/svg")));
    lay->addWidget(check);

    lay->addWidget(mk_checkbox(QWidget::tr("SVG"), "image-svg+xml", &::svg, wid));
    lay->addWidget(mk_checkbox(QWidget::tr("JSON"), "application-json", &::json, wid));
    lay->addWidget(mk_checkbox(QWidget::tr("PNG"), "image-png", &::png, wid));
    lay->insertStretch(-1);

    return wid;
}

bool ClipboardSettings::json()
{
    return ::json;
}

bool ClipboardSettings::png()
{
    return ::png;
}

bool ClipboardSettings::svg()
{
    return ::svg;
}


