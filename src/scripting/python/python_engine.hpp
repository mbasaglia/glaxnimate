#include <memory>
#include "scripting/script_engine.hpp"


namespace scripting::python {

class PythonContext : public ScriptExecutionContext
{
public:
    PythonContext();
    ~PythonContext();

    void expose(const QString& name, QObject* obj);

    QString eval_to_string(const QString& code);

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
        return std::make_unique<PythonContext>();
    }

private:
    static Autoregister<PythonEngine> autoreg;
};

} // namespace scripting::python

