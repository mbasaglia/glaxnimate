#pragma once
#include <QDialog>
#include <QVariant>
#include <QDir>

#include "glaxnimate/core/plugin/plugin.hpp"

namespace glaxnimate::gui {

class PluginUiDialog : public QDialog
{
    Q_OBJECT

public:
    PluginUiDialog(QIODevice& file, const plugin::Plugin& data, QWidget* parent = nullptr);

    Q_INVOKABLE QVariant get_value(const QString& widget, const QString& property);
    Q_INVOKABLE bool set_value(const QString& widget, const QString& property, const QVariant& value);
};

} // namespace glaxnimate::gui
