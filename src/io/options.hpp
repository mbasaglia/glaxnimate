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

    Option(QString slug, QString name, QString description)
        : type(Info),
        slug(std::move(slug)),
        name(std::move(name)),
        description(std::move(description))
    {}

    Option(QString slug, QString name, QString description, bool default_value)
        : type(Bool),
        slug(std::move(slug)),
        name(std::move(name)),
        description(std::move(description)),
        default_value(default_value)
    {}

    Option(QString slug, QString name, QString description, int default_value, int min, int max)
        : type(Int),
        slug(std::move(slug)),
        name(std::move(name)),
        description(std::move(description)),
        default_value(default_value),
        min(min),
        max(max)
    {}

    Option(QString slug, QString name, QString description, float default_value, float min, float max)
        : type(Float),
        slug(std::move(slug)),
        name(std::move(name)),
        description(std::move(description)),
        default_value(default_value),
        min(min),
        max(max)
    {}

    Option(QString slug, QString name, QString description, const QString& default_value)
        : type(String), slug(std::move(slug)), name(std::move(name)),
        description(std::move(description)), default_value(default_value)
    {}

    Option(QString slug, QString name, QString description, Type type, QVariant default_value)
        : type(type), slug(std::move(slug)), name(std::move(name)),
        description(std::move(description)), default_value(std::move(default_value))
    {}

    QVariant get_variant(const QVariantMap& map) const
    {
        auto it = map.find(slug);
        if ( it != map.end() && valid_variant(*it) )
            return *it;
        return default_value;
    }

    template<class CastType>
    CastType get(const QVariantMap& map) const
    {
        return get_variant(map).value<CastType>();
    }

    bool valid_variant(const QVariant& v) const
    {
        switch ( type )
        {
            case Info:
                return true;
            case Bool:
                return v.canConvert<bool>();
            case Int:
                return v.canConvert<int>();
            case Float:
                return v.canConvert<float>();
            case String:
                return v.canConvert<QString>();
            default:
                return false;
        }
    }

    Type type;
    QString slug;
    QString name;
    QString description;
    QVariant default_value;
    float min = 0;
    float max = 0;

};

using OptionList = std::vector<Option>;

} // namespace io
