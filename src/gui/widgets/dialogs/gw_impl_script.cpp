#include "glaxnimate_window_p.hpp"
#include "widgets/dialogs/plugin_ui_dialog.hpp"
#include "plugin/action.hpp"

void GlaxnimateWindow::Private::console_stderr(const QString& line)
{
    ui.console_output->setTextColor(Qt::red);
    ui.console_output->append(line);
}

void GlaxnimateWindow::Private::console_stdout(const QString& line)
{
    ui.console_output->setTextColor(parent->palette().text().color());
    ui.console_output->append(line);
}

void GlaxnimateWindow::Private::console_error(const app::scripting::ScriptError& err)
{
    console_stderr(err.message());
}

void GlaxnimateWindow::Private::console_commit(QString text)
{
    if ( text.isEmpty() )
        return;

    text = text.replace("\n", " ");

    if ( !ensure_script_contexts() )
        return;

    auto c = ui.console_output->textCursor();

    console_stdout("> " + text);
    auto ctx = script_contexts[ui.console_language->currentIndex()].get();
    try {
        QString out = ctx->eval_to_string(text);
        if ( !out.isEmpty() )
            console_stdout(out);
    } catch ( const app::scripting::ScriptError& err ) {
        console_error(err);
    }
    ui.console_input->setText("");


    c.clearSelection();
    c.movePosition(QTextCursor::End);
    ui.console_output->setTextCursor(c);
}

bool GlaxnimateWindow::Private::ensure_script_contexts()
{
    if ( script_contexts.empty() )
    {
        create_script_context();
        if ( script_contexts.empty() )
            return false;
    }

    return true;
}

void GlaxnimateWindow::Private::create_script_context()
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
            ctx->expose("window", QVariant::fromValue(parent));
            ctx->expose("document", QVariant::fromValue(current_document.get()));
        } catch ( const app::scripting::ScriptError& err ) {
            console_error(err);
        }

        script_contexts.push_back(std::move(ctx));
    }
}

bool GlaxnimateWindow::Private::execute_script ( const plugin::Plugin& plugin, const plugin::PluginScript& script, const QVariantList& args )
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
                    show_warning(plugin.data().name, tr("Could not run the plugin"), app::log::Error);
            } catch ( const app::scripting::ScriptError& err ) {
                console_error(err);
                show_warning(plugin.data().name, tr("Plugin raised an exception"), app::log::Error);
                ok = false;
            }
            current_plugin = nullptr;
            return ok;
        }
    }

    show_warning(plugin.data().name, tr("Could not find an interpreter"), app::log::Error);
    return false;
}

PluginUiDialog * GlaxnimateWindow::Private::create_dialog(const QString& ui_file)
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


void GlaxnimateWindow::Private::init_plugins()
{
    auto& par = plugin::PluginActionRegistry::instance();
    for ( auto act : par.enabled() )
    {
        ui.menu_plugins->addAction(par.make_qaction(act));
    }
    connect(&par, &plugin::PluginActionRegistry::action_added, parent, [this](plugin::ActionService* action) {
        ui.menu_plugins->addAction(plugin::PluginActionRegistry::instance().make_qaction(action));
    });
    connect(
        &plugin::PluginRegistry::instance(),
        &plugin::PluginRegistry::loaded,
        parent,
        &GlaxnimateWindow::script_reloaded
    );

    plugin::PluginRegistry::instance().set_executor(parent);
}
