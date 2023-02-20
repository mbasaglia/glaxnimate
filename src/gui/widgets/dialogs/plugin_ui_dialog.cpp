/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "plugin_ui_dialog.hpp"

#include <QMetaObject>
#include <QVBoxLayout>
#include <QtUiTools/QUiLoader>
#include <QDialogButtonBox>

#include "app/log/log.hpp"
#include "app/application.hpp"

#include <QtColorWidgets/ColorSelector>
#include <QtColorWidgets/color_2d_slider.hpp>
#include <QtColorWidgets/ColorDialog>
#include <QtColorWidgets/ColorListWidget>
#include <QtColorWidgets/ColorPreview>
#include <QtColorWidgets/ColorWheel>
#include <QtColorWidgets/color_palette_widget.hpp>
#include <QtColorWidgets/GradientEditor>
#include <QtColorWidgets/GradientSlider>
#include <QtColorWidgets/HueSlider>
#include <QtColorWidgets/swatch.hpp>
#include <QtColorWidgets/HarmonyColorWheel>

using namespace glaxnimate::gui;
using namespace glaxnimate;

/**
 * On mac widgets created from the plugin don't work right, so we fake it...
 */
class ColoredUiLoader : public QUiLoader
{
public:
    using QUiLoader::QUiLoader;

    QWidget* createWidget(const QString & className, QWidget * parent, const QString & name) override
    {
        if ( className.startsWith("color_widgets::") )
        {
            auto it = ctors().find(className);
            if ( it != ctors().end() )
            {
                QWidget* widget = it->second->create(parent);
                widget->setObjectName(name);
                return widget;

            }
        }
        return QUiLoader::createWidget(className, parent, name);
    }

private:
    class CtorBase
    {
    public:
        virtual ~CtorBase() = default;
        virtual QWidget* create(QWidget* parent) const = 0;
    };

    template<class Wid>
    class Ctor : public CtorBase
    {
    public:
        QWidget* create(QWidget* parent) const override
        {
            return new Wid(parent);
        }

        static std::pair<QString, std::unique_ptr<CtorBase>> pair()
        {
            return {
                Wid::staticMetaObject.className(),
                std::make_unique<Ctor>()
            };
        }
    };

    static const std::map<QString, std::unique_ptr<CtorBase>>& ctors()
    {
        static std::map<QString, std::unique_ptr<CtorBase>> meta;
        if ( meta.empty() )
        {
            meta.insert(Ctor<color_widgets::Color2DSlider>::pair());
            meta.insert(Ctor<color_widgets::ColorSelector>::pair());
            meta.insert(Ctor<color_widgets::ColorDialog>::pair());
            meta.insert(Ctor<color_widgets::ColorListWidget>::pair());
            meta.insert(Ctor<color_widgets::ColorPreview>::pair());
            meta.insert(Ctor<color_widgets::ColorWheel>::pair());
            meta.insert(Ctor<color_widgets::ColorPaletteWidget>::pair());
            meta.insert(Ctor<color_widgets::GradientEditor>::pair());
            meta.insert(Ctor<color_widgets::GradientSlider>::pair());
            meta.insert(Ctor<color_widgets::HueSlider>::pair());
            meta.insert(Ctor<color_widgets::Swatch>::pair());
            meta.insert(Ctor<color_widgets::HarmonyColorWheel>::pair());
        }

        return meta;
    }
};

PluginUiDialog::PluginUiDialog(QIODevice& file, const plugin::Plugin& data, QWidget* parent)
    : QDialog(parent)
{
    ColoredUiLoader loader;
    loader.setWorkingDirectory(data.data().dir);
    loader.setLanguageChangeEnabled(true);
    // for ( const auto& dir : app::Application::instance()->data_paths("lib") )
        // loader.addPluginPath(dir);

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
