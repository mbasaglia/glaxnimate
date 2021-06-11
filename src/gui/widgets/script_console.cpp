#include "script_console.hpp"
#include "ui_script_console.h"

#include <QEvent>


#include "app/settings/settings.hpp"
#include "app/scripting/script_engine.hpp"
#include "plugin/plugin.hpp"
#include "widgets/dialogs/plugin_ui_dialog.hpp"

class ScriptConsole::Private
{
public:
    Ui::ScriptConsole ui;

    std::vector<app::scripting::ScriptContext> script_contexts;
    const plugin::Plugin* current_plugin = nullptr;
    ScriptConsole* parent;
    std::map<QString, QVariant> globals;


    bool ensure_script_contexts()
    {
        if ( script_contexts.empty() )
        {
            create_script_context();
            if ( script_contexts.empty() )
                return false;
        }

        return true;
    }

    bool execute_script ( const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& args )
    {
        if ( !ensure_script_contexts() )
            return false;

        for ( const auto& ctx : script_contexts )
        {
            if ( ctx->engine() == plugin.data().engine )
            {
                current_plugin = &plugin;
                bool ok = false;
                try {
                    ok = ctx->run_from_module(plugin.data().dir, script.module, script.function, args);
                    if ( !ok )
                        parent->error(plugin.data().name, tr("Could not run the plugin"));
                } catch ( const app::scripting::ScriptError& err ) {
                    console_error(err);
                    parent->error(plugin.data().name, tr("Plugin raised an exception"));
                    ok = false;
                }
                current_plugin = nullptr;
                return ok;
            }
        }

        parent->error(plugin.data().name, tr("Could not find an interpreter"));
        return false;
    }

    void run_snippet(const QString& text, bool echo)
    {
        if ( !ensure_script_contexts() )
            return;

        auto c = ui.console_output->textCursor();

        if ( echo )
            console_stdout("> " + text);

        auto ctx = script_contexts[ui.console_language->currentIndex()].get();
        try {
            QString out = ctx->eval_to_string(text);
            if ( !out.isEmpty() )
                console_stdout(out);
        } catch ( const app::scripting::ScriptError& err ) {
            console_error(err);
        }

        c.clearSelection();
        c.movePosition(QTextCursor::End);
        ui.console_output->setTextCursor(c);
    }

    void console_commit(QString text)
    {
        if ( text.isEmpty() )
            return;


        run_snippet(text.replace("\n", " "), true);

        ui.console_input->setText("");
    }


    void console_stderr(const QString& line)
    {
        ui.console_output->setTextColor(Qt::red);
        ui.console_output->append(line);
    }

    void console_stdout(const QString& line)
    {
        ui.console_output->setTextColor(parent->palette().text().color());
        ui.console_output->append(line);
    }

    void console_error(const app::scripting::ScriptError& err)
    {
        console_stderr(err.message());
    }

    void create_script_context()
    {
        for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
        {
            auto ctx = engine->create_context();

            if ( !ctx )
                continue;

            connect(ctx.get(), &app::scripting::ScriptExecutionContext::stdout_line, [this](const QString& s){ console_stdout(s);});
            connect(ctx.get(), &app::scripting::ScriptExecutionContext::stderr_line, [this](const QString& s){ console_stderr(s);});

            try {
                ctx->app_module("glaxnimate");
                ctx->app_module("glaxnimate_gui");
                for ( const auto& p : globals )
                    ctx->expose(p.first, p.second);
            } catch ( const app::scripting::ScriptError& err ) {
                console_error(err);
            }

            script_contexts.push_back(std::move(ctx));
        }
    }

    PluginUiDialog * create_dialog(const QString& ui_file)
    {
        if ( !current_plugin )
            return nullptr;

        if ( !current_plugin->data().dir.exists(ui_file) )
        {
            current_plugin->logger().stream(app::log::Error) << "UI file not found:" << ui_file;
            return nullptr;
        }

        QFile file(current_plugin->data().dir.absoluteFilePath(ui_file));
        if ( !file.open(QIODevice::ReadOnly) )
        {
            current_plugin->logger().stream(app::log::Error) << "Could not open UI file:" << ui_file;
            return nullptr;
        }

        return new PluginUiDialog(file, *current_plugin, parent);
    }
};

ScriptConsole::ScriptConsole(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->parent = this;


    d->ui.console_input->setHistory(app::settings::get<QStringList>("scripting", "history"));

    for ( const auto& engine : app::scripting::ScriptEngineFactory::instance().engines() )
    {
        d->ui.console_language->addItem(engine->label());
        if ( engine->slug() == "python" )
            d->ui.console_language->setCurrentIndex(d->ui.console_language->count()-1);
    }

    connect(d->ui.btn_reload, &QAbstractButton::clicked, this, &ScriptConsole::clear_contexts);
}

ScriptConsole::~ScriptConsole() = default;

void ScriptConsole::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void ScriptConsole::console_clear()
{
    d->ui.console_output->clear();
}

void ScriptConsole::console_commit(const QString& command)
{
    d->console_commit(command);
}

bool ScriptConsole::execute(const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& args)
{
    return d->execute_script(plugin, script, args);
}

QVariant ScriptConsole::get_global(const QString& name)
{
    auto it = d->globals.find(name);
    if ( it != d->globals.end() )
        return it->second;
    return {};
}

void ScriptConsole::set_global(const QString& name, const QVariant& value)
{
    d->globals[name] = value;
}

void ScriptConsole::clear_contexts()
{
    d->script_contexts.clear();
    if ( !d->ui.check_persist->isChecked() )
        console_clear();
}

PluginUiDialog* ScriptConsole::create_dialog(const QString& ui_file) const
{
    return d->create_dialog(ui_file);
}

void ScriptConsole::save_settings()
{
    QStringList history = d->ui.console_input->history();
    int max_history = app::settings::get<int>("scripting", "max_history");
    if ( history.size() > max_history )
        history.erase(history.begin(), history.end() - max_history);
    app::settings::set("scripting", "history", history);
}

void ScriptConsole::run_snippet(const QString& source)
{
    d->run_snippet(source, false);
}
