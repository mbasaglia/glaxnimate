#pragma once

#include <vector>

#include <QVariant>
#include <QString>

namespace io {

struct Option
{
    enum Type
    {
        Info,
        Bool,
        Int,
        Float,
        String
    };

    Option(QString name, QString description)
        : type(Info), name(std::move(name)), description(std::move(description))
    {}

    Option(QString name, QString description, bool default_value)
        : type(Bool), name(std::move(name)), description(std::move(description)), default_value(default_value)
    {}

    Option(QString name, QString description, int default_value)
        : type(Int), name(std::move(name)), description(std::move(description)), default_value(default_value)
    {}

    Option(QString name, QString description, float default_value)
        : type(Float), name(std::move(name)), description(std::move(description)), default_value(default_value)
    {}

    Option(QString name, QString description, const QString& default_value)
        : type(String), name(std::move(name)), description(std::move(description)), default_value(default_value)
    {}

    Option(QString name, QString description, Type type, QVariant default_value)
        : type(type), name(std::move(name)), description(std::move(description)), default_value(std::move(default_value))
    {}


    Type type;
    QString name;
    QString description;
    QVariant default_value;

};

using OptionList = std::vector<Option>;

} // namespace io
