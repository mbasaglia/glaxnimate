#include "emoji_set.hpp"

glaxnimate::emoji::GitHubTemplate glaxnimate::emoji::GitHubTemplate::load(const QJsonObject& json)
{
    GitHubTemplate obj;
    obj.repo = json["repo"].toString();
    obj.org = json["org"].toString();
    obj.branch = json["branch"].toString();
    return obj;
}

void glaxnimate::emoji::GitHubTemplate::apply(EmojiSet& set) const
{
    set.url = "https://github.com/" + org + "/" + repo;
    set.download.url = "https://github.com/" + org + "/" + repo + "/archive/refs/heads/" + branch + ".tar.gz";
    for ( auto& path : set.download.paths )
    {
        if ( path.second.size == EmojiSetDirectory::Scalable )
            set.preview_template = "https://raw.githubusercontent.com/" + org + "/" + repo + "/" + branch + "/" + path.second.path + "%1.svg";
        path.second.path = repo + "-" + branch + "/" + path.second.path;
    }
}

glaxnimate::emoji::EmojiSet glaxnimate::emoji::EmojiSet::load(const QJsonObject& json)
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
    if ( json.contains("template") )
        GitHubTemplate::load(json["template"].toObject()).apply(obj);

    return obj;
}
