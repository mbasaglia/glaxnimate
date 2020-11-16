#pragma once

#include <vector>
#include <functional>

#include <QVariant>
#include <QString>

namespace app::settings {

struct Setting
{
    enum Type
    {
        Internal,
        Info,
        Bool,
        Int,
        Float,
        String,
        Color,
    };

    Setting(QString slug, QString label, QString description)
        : type(Info),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description))
    {}

    Setting(QString slug, QString label, QString description, bool default_value)
        : type(Bool),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description)),
        default_value(default_value)
    {}

    Setting(QString slug, QString label, QString description, int default_value, int min, int max)
        : type(Int),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description)),
        default_value(default_value),
        min(min),
        max(max)
    {}

    Setting(QString slug, QString label, QString description, float default_value, float min, float max)
        : type(Float),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description)),
        default_value(default_value),
        min(min),
        max(max)
    {}

    Setting(QString slug, QString label, QString description, const QString& default_value)
        : type(String), slug(std::move(slug)), label(std::move(label)),
        description(std::move(description)), default_value(default_value)
    {}

    Setting(QString slug, QString label, QString description, Type type,
            QVariant default_value, QVariantMap choices = {},
            std::function<void(const QVariant&)> side_effects = {}
           )
        : type(type),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description)),
        default_value(std::move(default_value)),
        choices(std::move(choices)),
        side_effects(std::move(side_effects))
    {}

    Setting(QString slug, QString label, QString description, const QColor& default_value)
        : type(Color),
        slug(std::move(slug)),
        label(std::move(label)),
        description(std::move(description)),
        default_value(QVariant::fromValue(default_value))
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
            case Internal:
                return true;
            case Bool:
                return v.canConvert<bool>();
            case Int:
                return v.canConvert<int>();
            case Float:
                return v.canConvert<float>();
            case String:
                return v.canConvert<QString>();
            case Color:
                return v.canConvert<QColor>();
            default:
                return false;
        }
    }

    Type type;
    QString slug;
    QString label;
    QString description;
    QVariant default_value;
    float min = -1;
    float max = -1;
    QVariantMap choices;
    std::function<void(const QVariant&)> side_effects;

};

using SettingList = std::vector<Setting>;

} // namespace app::settings
