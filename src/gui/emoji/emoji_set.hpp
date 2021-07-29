#pragma once

#include <vector>
#include <map>

#include <QString>
#include <QUrl>
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>

#include "emoji_data.hpp"


namespace glaxnimate::emoji {

struct EmojiSetDirectory
{
    static constexpr const int Scalable = -1;
    QString path;
    QString format;
    int size = Scalable;

    static EmojiSetDirectory load(const QJsonObject& json)
    {
        EmojiSetDirectory obj;
        obj.path = json["path"].toString();
        obj.format = json["format"].toString();
        if ( obj.format != "svg" )
            obj.size = json["size"].toInt();
        return obj;
    }
};

struct EmojiSetDownload
{
    QUrl url;
    std::map<int, EmojiSetDirectory> paths;

    static EmojiSetDownload load(const QJsonObject& json)
    {
        EmojiSetDownload obj;
        obj.url = json["url"].toString();
        for ( const auto& val : json["paths"].toArray() )
        {
            auto path = EmojiSetDirectory::load(val.toObject());
            int size = path.size;
            obj.paths.emplace(size, std::move(path));
        }
        return obj;
    }
};

struct EmojiSetSlugFormat
{
    bool upper = false;
    QChar separator = '-';
    QString prefix;

    QString slug(const Emoji& emoji) const
    {
        return slug(emoji.hex_slug);
    }

    QString slug(QString slug) const
    {
        if ( upper )
            slug = slug.toUpper();
        if ( separator != '-' )
            slug = slug.replace('-', separator);
        return prefix + slug;
    }
};

struct EmojiSet
{
    QString name;
    QUrl url;
    QString license;
    QString preview_template;
    EmojiSetSlugFormat slug;
    EmojiSetDownload download;
    QDir path;

    static EmojiSet load(const QJsonObject& json)
    {
        EmojiSet obj;
        obj.name = json["name"].toString();
        obj.url = json["url"].toString();
        obj.license = json["license"].toString();
        obj.preview_template = json["preview"].toString();
        QString slug_format = json["slug_format"].toString();
        obj.slug.upper = slug_format[0].isUpper();
        obj.slug.separator = slug_format.back();
        obj.slug.prefix = json["slug_prefix"].toString();

        obj.download = EmojiSetDownload::load(json["download"].toObject());

        return obj;
    }

    QString image_path(int size, const Emoji& emoji) const
    {
        return image_path(size, emoji.hex_slug);
    }

    QString image_path(int size, const QString& hex_slug) const
    {
        const auto& path_data = download.paths.at(size);
        return path.absoluteFilePath(path_data.path + "/" + slug.slug(hex_slug) + "." + path_data.format);
    }

    QDir image_path(int size) const
    {
        return path.absoluteFilePath(download.paths.at(size).path);
    }

    QUrl preview_url(const Emoji& emoji) const
    {
        return preview_url(emoji.hex_slug);
    }

    QUrl preview_url(const QString& hex_slug) const
    {
        return preview_template.arg(slug.slug(hex_slug));
    }
};

} // namespace glaxnimate::emoji
