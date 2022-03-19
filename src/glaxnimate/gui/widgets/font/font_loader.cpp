#include "font_loader.hpp"

#include <set>

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QRegularExpression>

#include "glaxnimate/svg/css_parser.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/core/model/assets/pending_asset.hpp"




class glaxnimate::gui::font::FontLoader::Private
{
public:
    struct QueueItem
    {
        int id = -2;
        QString name_alias;
        QUrl url;
        QUrl parent_url = {};
    };

    QNetworkAccessManager downloader;
    std::vector<model::CustomFont> fonts;
    int resolved = 0;
    std::vector<QueueItem> queued;
    std::set<QNetworkReply*> active_replies;
    bool loading = false;
    FontLoader* parent;

    void load_file(const QueueItem& item)
    {
        QFile file(item.url.toLocalFile());
        if ( !file.open(QFile::ReadOnly) )
            parent->error(tr("Could not open file %1").arg(file.fileName()), item.id);
        else
            parse(item.name_alias, item.id, file.readAll(), item.parent_url, item.url);

        mark_resolved();
    }

    void load_data(const QueueItem& item)
    {
        auto info = item.url.path().split(";");
        if ( info.empty() || !info.back().startsWith("base64,") )
            parent->error(tr("Invalid data URL"), item.id);
        else
            parse(item.name_alias, item.id, QByteArray::fromBase64(info.back().mid(7).toLatin1(), QByteArray::Base64UrlEncoding), item.parent_url, item.url);
        mark_resolved();
    }

    void load_item(const QueueItem& item)
    {
        if ( item.url.isLocalFile() )
            load_file(item);
        else if ( item.url.scheme() == "data" )
            load_data(item);
        else
            request(item);

    }

    void request(const QueueItem& item)
    {
        QNetworkRequest request(item.url);
        request.setMaximumRedirectsAllowed(3);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

        QNetworkReply* response = downloader.get(request);
        response->setProperty("css_url", item.parent_url);
        response->setProperty("id", item.id);
        response->setProperty("name_alias", item.name_alias);
        active_replies.insert(response);
    }

    void mark_resolved()
    {
        resolved++;
        emit parent->fonts_loaded(resolved);
        if ( resolved >= int(queued.size()) )
            emit parent->finished();
    }

    void handle_response(QNetworkReply *reply)
    {
        if ( reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 )
        {
            auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            emit parent->error(reason, reply->property("id").toInt());
        }
        else
        {
            parse(
                reply->property("name_alias").toString(),
                reply->property("id").toInt(),
                reply->readAll(),
                reply->property("css").toUrl(),
                reply->url()
            );
            reply->close();
        }

        active_replies.erase(reply);
        mark_resolved();
    }

    void parse(const QString& name_alias, int id, const QByteArray& data, const QUrl& css_url, const QUrl& reply_url)
    {
        switch ( model::CustomFontDatabase::font_data_format(data) )
        {
            case model::FontFileFormat::TrueType:
            case model::FontFileFormat::OpenType:
            {
                model::CustomFont font = model::CustomFontDatabase::instance().add_font(name_alias, data);
                font.set_source_url(reply_url.toString());
                font.set_css_url(css_url.toString());
                fonts.push_back(font);
                if ( id != -1 )
                    emit parent->success(id);
                break;
            }
            case model::FontFileFormat::Unknown:
                parse_css(id, data, reply_url);
                break;
            default:
                parent->error(tr("Font format not supported for %1").arg(reply_url.toString()), id);
        }
    }

    void parse_css(int id, const QByteArray& data, const QUrl& css_url)
    {
        if ( !data.contains("@font-face") )
            return;

        std::vector<io::svg::detail::CssStyleBlock> blocks;
        io::svg::detail::CssParser parser(blocks);
        parser.parse(QString(data));
        static QRegularExpression url(R"(url\s*\(\s*(['"]?)([^'")]+)(\1)\s*\))");
        std::set<QString> urls;

        for ( auto& block : blocks )
        {
            if ( block.selector.at_rule() == "@font-face" )
            {
                auto match = url.match(block.style["src"]);
                if ( match.hasMatch() )
                {
                    QString url = match.captured(2);
                    if ( urls.insert(url).second )
                    {
                        QString fam = block.style["font-family"];
                        if ( fam.size() > 1 && (fam[0] == '"' || fam[0] == '\'') )
                            fam = fam.mid(1, fam.size() - 2);
                        queue(QueueItem{-1, fam, url, css_url});
                    }
                }
            }
        }


        emit parent->fonts_queued(queued.size());

        if ( id != -1 )
            emit parent->success(id);
    }

    void queue(const QueueItem& item)
    {
        for ( const auto& other : queued )
            if ( other.url == item.url )
                return;

        queued.push_back(item);

        if ( loading )
        {
            load_item(item);
            emit parent->fonts_queued(queued.size());
        }
    }
};

glaxnimate::gui::font::FontLoader::FontLoader()
    : d(std::make_unique<Private>())
{
    d->parent = this;
    d->downloader.setParent(this);
    connect(&d->downloader, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply){
        d->handle_response(reply);
    });
}

glaxnimate::gui::font::FontLoader::~FontLoader()
{
}

void glaxnimate::gui::font::FontLoader::clear()
{
    for ( auto reply : d->active_replies )
        reply->abort();

    d->active_replies.clear();
    d->fonts.clear();
    d->resolved = 0;
    d->queued.clear();
    d->loading = false;

    emit fonts_queued(0);
    emit fonts_loaded(0);
}

void glaxnimate::gui::font::FontLoader::load_queue()
{
    d->loading = true;
    for ( const auto& url : d->queued )
        d->load_item(url);

    emit fonts_queued(d->queued.size());
    emit fonts_loaded(0);
}

void glaxnimate::gui::font::FontLoader::queue(const QString& name_alias, const QUrl& url, int id)
{
    d->queue({id, name_alias, url});
}

int glaxnimate::gui::font::FontLoader::queued_total() const
{
    return d->queued.size();
}

const std::vector<glaxnimate::model::CustomFont> & glaxnimate::gui::font::FontLoader::fonts() const
{
    return d->fonts;
}

void glaxnimate::gui::font::FontLoader::queue_data(const QString& name_alias, const QByteArray& data, int id)
{
    d->parse(name_alias, id, data, {}, {});
}


void glaxnimate::gui::font::FontLoader::cancel()
{
    if ( d->loading )
    {
        for ( const auto& reply : d->active_replies )
            reply->abort();
        clear();
    }
}

bool glaxnimate::gui::font::FontLoader::loading() const
{
    return d->loading;
}

void glaxnimate::gui::font::FontLoader::queue_pending(model::Document* document, bool reload_loaded)
{
    connect(this, &FontLoader::success, document, &model::Document::mark_asset_loaded);

    for ( const auto& pending : document->pending_assets() )
    {
        if ( reload_loaded || !pending.loaded )
        {
            if ( pending.url.isValid() )
                queue(pending.name_alias, pending.url, pending.id);
            else if ( !pending.data.isEmpty() )
                queue_data(pending.name_alias, pending.data, pending.id);
        }
    }
}
