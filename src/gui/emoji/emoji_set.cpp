/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "emoji_set.hpp"

glaxnimate::emoji::GitHubTemplate glaxnimate::emoji::GitHubTemplate::load(const QJsonObject& json)
{
    GitHubTemplate obj;
    obj.repo = json["repo"].toString();
    obj.org = json["org"].toString();
    if ( json.contains("tag") )
    {
        obj.ref_type = Tag;
        obj.ref = json["tag"].toString();
    }
    else
    {
        obj.ref_type = Branch;
        obj.ref = json["branch"].toString();
    }
    return obj;
}

void glaxnimate::emoji::GitHubTemplate::apply(EmojiSet& set) const
{
    set.url = QUrl("https://github.com/" + org + "/" + repo);
    QString ref_url;
    QString ref_path = ref;
    if ( ref_type == Branch )
    {
        ref_url = "heads/";
    }
    else
    {
        ref_url = "tags/";
        if ( ref.size() && ref[0] == 'v' )
            ref_path.remove(0, 1);
    }
    set.download.url = QUrl("https://github.com/" + org + "/" + repo + "/archive/refs/" + ref_url + ref + ".tar.gz");
    for ( auto& path : set.download.paths )
    {
        if ( path.second.size == EmojiSetDirectory::Scalable )
            set.preview_template = "https://raw.githubusercontent.com/" + org + "/" + repo + "/" + ref + "/" + path.second.path + "%1.svg";
        path.second.path = repo + "-" + ref_path + "/" + path.second.path;
    }
}

glaxnimate::emoji::EmojiSet glaxnimate::emoji::EmojiSet::load(const QJsonObject& json)
{
    EmojiSet obj;
    obj.name = json["name"].toString();
    obj.url = QUrl(json["url"].toString());
    obj.license = json["license"].toString();
    obj.preview_template = json["preview"].toString();
    QString slug_format = json["slug_format"].toString();
    obj.slug.upper = slug_format[0].isUpper();
    obj.slug.separator = slug_format.back();
    obj.slug.prefix = json["slug_prefix"].toString();

    obj.download = EmojiSetDownload::load(json["download"].toObject());
    if ( json.contains("template") )
        GitHubTemplate::load(json["template"].toObject()).apply(obj);

    return obj;
}
