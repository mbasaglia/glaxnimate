#include <memory>
#include "scripting/script_engine.hpp"


namespace scripting::python {

class PythonContext : public ScriptExecutionContext
{
public:
    PythonContext(const ScriptEngine* engine);
    ~PythonContext();

    void expose(const QString& name, const QVariant& obj) override;

    QString eval_to_string(const QString& code) override;

    bool run_from_module (
        const QDir& path,
        const QString& module,
        const QString& function,
        const QVariantList& args
    ) override;


    const ScriptEngine* engine() const override;


private:
    class Private;

    std::unique_ptr<Private> d;
};

class PythonEngine : public ScriptEngine
{
public:
    QString slug() const override { return "python"; }
    QString label() const override { return "Python"; }

    ScriptContext create_context() const override
    {
        return std::make_unique<PythonContext>(this);
    }

private:
    static Autoregister<PythonEngine> autoreg;
};

} // namespace scripting::python

