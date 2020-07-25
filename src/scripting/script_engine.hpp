#pragma once

#include <stdexcept>
#include <memory>

#include <QString>
#include <QObject>
#include <QHash>


namespace scripting {

class ScriptError : public std::runtime_error
{
public:
    ScriptError(const QString& what) : std::runtime_error(what.toStdString()) {}

    QString message() const
    {
        return QString(what());
    }
};

/**
 * \brief Context for running scripts (implementation)
 */
class ScriptExecutionContext : public QObject
{
public:

    ScriptExecutionContext() = default;
    virtual ~ScriptExecutionContext() = default;

    /**
     * \brief Exposes \p obj as a global variable called \p name
     */
    virtual void expose(const QString& name, QObject* obj) = 0;

    /**
     * \brief Evaluates \p code and serializes the result (if any) into a string
     * \throws ScriptError on errors with the script
     */
    virtual QString eval_to_string(const QString& code) = 0;

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

} // namespace scripting
