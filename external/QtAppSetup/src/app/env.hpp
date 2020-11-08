#pragma once

#include <QtGlobal>
#include <QString>
#include <QStringList>

namespace app {

class Environment
{
public:
    class Variable
    {
    public:
        Variable(const char* name) : name(name) {}

        operator const QString&() const
        {
            ensure_loaded();
            return value;
        }

        const QString& get(const QString& default_value = {}) const
        {
            ensure_loaded(default_value);
            return value;
        }

        bool is_set() const
        {
            return qEnvironmentVariableIsSet(name);
        }

        void operator=(const QString& value) const
        {
            set(value);
        }

        void set(const QString& value) const
        {
            this->value = value;
            loaded = true;
            qputenv(name, value.toUtf8());
        }

        void erase() const
        {
            value.clear();
            loaded = true;
            qunsetenv(name);
        }

        const QString& load(const QString& default_value = {}) const
        {
            value = qEnvironmentVariable(name, default_value);
            loaded = true;
            return value;
        }

        bool operator==(const QString& val) const
        {
            return get() == val;
        }

        bool operator!=(const QString& val) const
        {
            return get() != val;
        }

        void push_back(const QString& item, const QString& separator=":")
        {
            ensure_loaded();
            if ( value.isEmpty() )
                value += separator;
            value += item;
            qputenv(name, value.toUtf8());
        }

        void push_back(const QStringList& items, const QString& separator=":")
        {
            if ( items.empty() )
                return;
            push_back(items.join(separator), separator);
        }

        void push_front(const QString& item, const QString& separator=":")
        {
            ensure_loaded();
            QString new_value = item;
            if ( value.isEmpty() )
                value += separator;
            new_value += value;
            set(new_value);
        }

        void push_front(const QStringList& items, const QString& separator=":")
        {
            if ( items.empty() )
                return;
            push_front(items.join(separator), separator);
        }

        QStringList to_list(const QString& separator=":") const
        {
            ensure_loaded();
            return value.split(separator);
        }

        bool empty() const
        {
            ensure_loaded({});
            return value.isEmpty();
        }

    private:
        void ensure_loaded(const QString& default_value = {}) const
        {
            if ( !loaded )
                load(default_value);
        }

        QByteArray name;
        mutable QString value;
        mutable bool loaded = false;
    };

    Variable operator[](const char* name) { return Variable(name); }
    bool contains(const char* name) { return qEnvironmentVariableIsSet(name); }
    void erase(const char* name) { qunsetenv(name); }
};

} // namespace app
