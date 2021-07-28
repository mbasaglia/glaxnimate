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
    QString archive_path;
    QString path;
    QString format;
    int size = Scalable;

    static EmojiSetDirectory load(const QJsonObject& json)
    {
        EmojiSetDirectory obj;
        obj.archive_path = json["path"].toString();
        obj.format = json["format"].toString();
        if ( obj.format != "svg" )
        {
            obj.size = json["size"].toInt();
            obj.path = QString(obj.size);
        }
        else
        {
            obj.path = obj.format;
        }
        return obj;
    }
};

struct EmojiSetDownload
{
    QUrl url;
    std::vector<EmojiSetDirectory> paths;

    static EmojiSetDownload load(const QJsonObject& json)
    {
        EmojiSetDownload obj;
        obj.url = json["url"].toString();
        for ( const auto& val : json["paths"].toArray() )
            obj.paths.push_back(EmojiSetDirectory::load(val.toObject()));
        return obj;
    }
};

struct EmojiSet
{
    QString name;
    QUrl url;
    QString license;
    QString preview_template;
    EmojiSetDownload download;
    bool slug_upper = false;
    QChar slug_separator = '-';
    QDir path;

    static EmojiSet load(const QJsonObject& json)
    {
        EmojiSet obj;
        obj.name = json["name"].toString();
        obj.url = json["url"].toString();
        obj.license = json["license"].toString();
        obj.preview_template = json["preview"].toString();
        QString slug_format = json["slug_format"].toString();
        obj.slug_upper = slug_format[0].isUpper();
        obj.slug_separator = slug_format.back();

        obj.download = EmojiSetDownload::load(json["download"].toObject());

        return obj;
    }

    QUrl preview_url(const Emoji& emoji) const
    {
        return preview_url(emoji.hex_slug);
    }

    QUrl preview_url(QString slug) const
    {
        if ( slug_upper )
            slug = slug.toUpper();
        if ( slug_separator != '-' )
            slug = slug.replace('-', slug_separator);
        return preview_template.arg(slug);
    }
};

} // namespace glaxnimate::emoji
