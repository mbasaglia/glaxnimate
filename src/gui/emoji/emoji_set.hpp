/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
        obj.url = QUrl(json["url"].toString());
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

struct GitHubTemplate
{
    enum RefType
    {
        Branch,
        Tag
    };
    QString repo;
    QString org;
    QString ref;
    RefType ref_type = Branch;

    static GitHubTemplate load(const QJsonObject& json);

    void apply(struct EmojiSet& set) const;
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

    static EmojiSet load(const QJsonObject& json);

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
        return QUrl(preview_template.arg(slug.slug(hex_slug)));
    }
};

} // namespace glaxnimate::emoji
