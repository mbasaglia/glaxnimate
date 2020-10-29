#include "plugin_ui_dialog.hpp"

#include <QtUiTools/QUiLoader>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "app/log/log.hpp"
#include "app/application.hpp"

PluginUiDialog::PluginUiDialog(QIODevice& file, const plugin::Plugin& data, QWidget* parent)
    : QDialog(parent)
{
    QUiLoader loader;
    loader.setWorkingDirectory(data.data().dir);
    loader.setLanguageChangeEnabled(true);
    for ( const auto& dir : app::Application::instance()->data_paths("lib") )
        loader.addPluginPath(dir);

    QWidget* child = loader.load(&file, nullptr);
    if ( !child )
    {
        data.logger().stream(app::log::Error) << "Could not load UI:" << loader.errorString();
        return;
    }

    if ( child->windowIcon().cacheKey() == qApp->windowIcon().cacheKey() )
        setWindowIcon(data.icon());
    else
        setWindowIcon(child->windowIcon());

    if ( child->windowTitle().isEmpty() )
        setWindowTitle(data.data().name);
    else
        setWindowTitle(child->windowTitle());

    resize(child->size());

    setSizePolicy(child->sizePolicy());


    if ( auto d = qobject_cast<QDialog*>(child) )
    {
        child->setVisible(false);
        connect(this, &QObject::destroyed, child, &QObject::deleteLater);
        connect(d, &QDialog::accepted, this, &QDialog::accept);
        connect(d, &QDialog::rejected, this, &QDialog::reject);
        setLayout(child->layout());
    }
    else
    {
        auto lay = new QVBoxLayout;
        setLayout(lay);
        lay->addWidget(child);
        auto box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        lay->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
}

QVariant PluginUiDialog::get_value(const QString& widget, const QString& property)
{
    QWidget* wid = findChild<QWidget*>(widget);
    if ( !wid )
        return {};

    return wid->property(property.toStdString().c_str());
}

bool PluginUiDialog::set_value(const QString& widget, const QString& property, const QVariant& value)
{
    QWidget* wid = findChild<QWidget*>(widget);
    if ( !wid )
        return {};

    return wid->setProperty(property.toStdString().c_str(), value);
}
