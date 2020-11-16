#pragma once

#include <stdexcept>
#include <memory>

#include <QString>
#include <QObject>
#include <QHash>
#include <QDir>
#include <QVariant>

#include "app/qstring_exception.hpp"


namespace app::scripting {

class ScriptError : public QStringException<>{ using Ctor::Ctor; };

class ScriptEngine;

/**
 * \brief Context for running scripts (implementation)
 */
class ScriptExecutionContext : public QObject
{
    Q_OBJECT
public:

    ScriptExecutionContext() = default;
    virtual ~ScriptExecutionContext() = default;

    /**
     * \brief Exposes \p obj as a global variable called \p name
     */
    virtual void expose(const QString& name, const QVariant& obj) = 0;

    /**
     * \brief Evaluates \p code and serializes the result (if any) into a string
     * \throws ScriptError on errors with the script
     */
    virtual QString eval_to_string(const QString& code) = 0;

    /**
     * \brief Marks an app module that must be loaded
     */
    virtual void app_module(const QString& name) = 0;

    /**
     * \brief Runs a function from a file
     * \return Whether the call was successful
     */
    virtual bool run_from_module (
        const QDir& path,       ///< Path containing the module file
        const QString& module,  ///< Module name to load, meaning of this depends on the engine
        const QString& function,///< Function to call
        const QVariantList& args///< Arguments to pass the function
    ) = 0;

    /**
     * \brief Engine that created the context
     */
    virtual const ScriptEngine* engine() const = 0;

signals:
    void stderr_line(const QString&);
    void stdout_line(const QString&);

private:
    ScriptExecutionContext(const ScriptExecutionContext&) = delete;
    ScriptExecutionContext& operator=(const ScriptExecutionContext&) = delete;
};


/**
 * \brief Script context holder
 */
using ScriptContext = std::unique_ptr<ScriptExecutionContext>;


/**
 * \brief Scripting system metadata
 */
class ScriptEngine
{
public:
    virtual ~ScriptEngine() = default;

    /**
     * \brief short machine-readable name for the language / engine
     */
    virtual QString slug() const = 0;

    /**
     * \brief Human-readable name
     */
    virtual QString label() const = 0;

    /**
     * \brief Creates an execution context to run scripts
     */
    virtual ScriptContext create_context() const = 0;

protected:
    template<class T>
    struct Autoregister;
};


class ScriptEngineFactory
{
public:
    static ScriptEngineFactory& instance()
    {
        static ScriptEngineFactory instance;
        return instance;
    }

    void register_engine(std::unique_ptr<ScriptEngine> eng)
    {
        engines_.push_back(std::move(eng));
    }

    const ScriptEngine* engine(const QString& slug) const
    {
        for ( const auto& engine: engines_ )
            if ( engine->slug() == slug )
                return engine.get();
        return nullptr;
    }

    const std::vector<std::unique_ptr<ScriptEngine>>& engines() const
    {
        return engines_;
    }

private:
    ScriptEngineFactory() = default;
    ScriptEngineFactory(const ScriptEngineFactory&) = delete;
    ~ScriptEngineFactory() = default;
    std::vector<std::unique_ptr<ScriptEngine>> engines_;
};

template<class T>
struct ScriptEngine::Autoregister
{
    template<class... Args>
    Autoregister(Args&&... args)
    {
        ScriptEngineFactory::instance().register_engine(std::make_unique<T>(std::forward<Args>(args)...));
    }
};

} // namespace app::scripting
