/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include "app/scripting/script_engine.hpp"


namespace app::scripting::python {

class PythonContext : public ScriptExecutionContext
{
public:
    PythonContext(const ScriptEngine* engine);
    ~PythonContext();

    void expose(const QString& name, const QVariant& obj) override;

    QString eval_to_string(const QString& code) override;

    QStringList eval_completions(const QString& line) override;

    bool run_from_module (
        const QDir& path,
        const QString& module,
        const QString& function,
        const QVariantList& args
    ) override;


    const ScriptEngine* engine() const override;

    void app_module(const QString& name) override;

private:
    class Private;

    std::unique_ptr<Private> d;
};

class PythonEngine : public ScriptEngine
{
public:
    static void add_module_search_paths(const QStringList& paths);

    QString slug() const override { return "python"; }
    QString label() const override { return "Python"; }

    ScriptContext create_context() const override;

private:
    static Autoregister<PythonEngine> autoreg;
};

} // namespace app::scripting::python
