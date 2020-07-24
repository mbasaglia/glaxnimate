#pragma once

#include <QQmlEngine>

#include "scripting/script_engine.hpp"

namespace scripting::js {

class JsContext : public ScriptExecutionContext
{
public:
    JsContext()
    {
        engine.installExtensions(QJSEngine::ConsoleExtension);
    }

    void expose(const QString& name, QObject* obj)
    {
        engine.globalObject().setProperty(name, engine.newQObject(obj));
        QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
    }

    QString eval_to_string(const QString& code)
    {
        return engine.evaluate(code+";").toString();
    }

private:
    QQmlEngine engine;
};

class JsEngine : public ScriptEngine
{
public:
    QString slug() const override { return "js"; }
    QString label() const override { return "ECMAScript"; }

    ScriptContext create_context() const override
    {
        return std::make_unique<JsContext>();
    }

private:
    static Autoregister<JsEngine> autoreg;
};

} // namespace scripting::js
