#pragma once

#include <QQmlEngine>

#include "app/scripting/script_engine.hpp"

namespace app::scripting::js {

class JsContext : public ScriptExecutionContext
{
public:
    JsContext(const ScriptEngine* js_engine) : js_engine(js_engine)
    {
        qml_engine.installExtensions(QJSEngine::ConsoleExtension);
    }

    QJSValue convert(const QVariant& val)
    {
        if ( val.userType() == QMetaType::QVariantMap )
        {
            auto jsv = qml_engine.newObject();
            auto cont = val.toMap();
            for ( auto it = cont.begin(); it != cont.end(); ++it )
                jsv.setProperty(it.key(), convert(*it));
            return jsv;
        }
        else if ( val.userType() == QMetaType::QVariantList )
        {
            auto cont = val.toList();
            auto jsv = qml_engine.newArray(cont.size());
            for ( int i = 0; i < cont.size(); i++ )
                jsv.setProperty(i, convert(cont[i]));
            return jsv;
        }
        else if ( val.type() == QVariant::Int )
        {
            return QJSValue(val.toInt());
        }
        else if ( val.type() == QVariant::String )
        {
            return QJSValue(val.toString());
        }
        else if ( val.canConvert<QObject*>() )
        {
            QObject* obj = val.value<QObject*>();
            if ( obj )
            {
                auto jsv = qml_engine.newQObject(obj);
                QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
                return jsv;
            }
        }
        return QJSValue{QJSValue::NullValue};
    }

    void expose(const QString& name, const QVariant& val) override
    {
        qml_engine.globalObject().setProperty(name, convert(val));
    }

    QString eval_to_string(const QString& code) override
    {
        return qml_engine.evaluate(code+";").toString();
    }

    bool run_from_module (
        const QDir& path,       ///< Path containing the module file
        const QString& module,  ///< Module name to load
        const QString& function,///< Function to call
        const QVariantList& args///< Arguments to pass the function
    ) override
    {
        QStringList import_path = qml_engine.importPathList();
        qml_engine.addImportPath(path.path());

        QJSValue module_obj = qml_engine.importModule(module);
        if ( module_obj.isError() || !module_obj.hasProperty(function) )
            return false;

        QJSValue func = module_obj.property(function);
        if ( !func.isCallable() )
            return false;

        QJSValueList jsargs;
        for ( const auto& arg : args )
            jsargs.push_back(convert(arg));

        QJSValue ret = func.call(jsargs);
        if ( ret.isError() )
            throw ScriptError(ret.toString());

        qml_engine.setImportPathList(import_path);
        return true;
    }

    const ScriptEngine* engine() const override
    {
        return js_engine;
    }

    void app_module(const QString&) override {}

private:
    QQmlEngine qml_engine;
    const ScriptEngine* js_engine;
};

class JsEngine : public ScriptEngine
{
public:
    QString slug() const override { return "js"; }
    QString label() const override { return "ECMAScript"; }

    ScriptContext create_context() const override
    {
        return std::make_unique<JsContext>(this);
    }

private:
    static Autoregister<JsEngine> autoreg;
};

} // namespace app::scripting::js
