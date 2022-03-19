#pragma once
#include <QString>
#include <QVariant>

namespace glaxnimate::plugin {
class Plugin;
class PluginScript;

class Executor
{
public:
    virtual bool execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& in_args) = 0;
    virtual QVariant get_global(const QString& name) = 0;
};

} // namespace glaxnimate::plugin
