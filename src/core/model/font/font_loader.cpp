#include "font_loader.hpp"

#include <set>

#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QRegularExpression>


#include "io/svg/css_parser.hpp"

glaxnimate::model::FontFileFormat glaxnimate::model::font_data_format(const QByteArray& data)
{
    QByteArray head = data.left(4);

    if ( head == "OTTO" )
        return FontFileFormat::OpenType;
    if ( head == QByteArray("\0\1\0\0", 4) )
        return FontFileFormat::TrueType;
    if ( head == "wOF2" )
        return FontFileFormat::Woff2;
    if ( head == "wOFF" )
        return FontFileFormat::Woff;

    return FontFileFormat::Unknown;
}

class glaxnimate::model::FontLoader::Private
{
public:
    struct QueueItem
    {
        QUrl url;
        QUrl parent_url = {};
    };

    QNetworkAccessManager downloader;
    std::vector<CustomFont> fonts;
    int resolved = 0;
    std::vector<QueueItem> queued;
    std::set<QNetworkReply*> active_replies;
    bool loading = false;
    FontLoader* parent;

    void load_file(const QueueItem& item)
    {
        QFile file(item.url.toLocalFile());
        if ( !file.open(QFile::ReadOnly) )
            parent->error(tr("Could not open file %1").arg(file.fileName()));
        else
            parse(file.readAll(), item.parent_url, item.url);

        mark_resolved();
    }

    void load_data(const QueueItem& item)
    {
        auto info = item.url.path().split(";");
        if ( info.empty() || !info.back().startsWith("base64,") )
            parent->error(tr("Invalid data URL"));
        else
            parse(QByteArray::fromBase64(info.back().mid(7).toLatin1(), QByteArray::Base64UrlEncoding), item.parent_url, item.url);
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
            emit parent->error(reason);
        }
        else
        {
            parse(reply->readAll(), reply->property("css").toUrl(), reply->url());
            reply->close();
        }

        active_replies.erase(reply);
        mark_resolved();
    }

    void parse(const QByteArray& data, const QUrl& css_url, const QUrl& reply_url)
    {
        switch ( font_data_format(data) )
        {
            case FontFileFormat::TrueType:
            case FontFileFormat::OpenType:
            {
                CustomFont font(data);
                font.set_source_url(reply_url.toString());
                font.set_css_url(css_url.toString());
                fonts.push_back(font);
                break;
            }
            case FontFileFormat::Unknown:
                return parse_css(data, reply_url);
            default:
                parent->error(tr("Font format not supported for %1").arg(reply_url.toString()));
        }
    }

    void parse_css(const QByteArray& data, const QUrl& css_url)
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
                    urls.insert(match.captured(2));
            }
        }

        for ( const auto& url : urls )
            queue(QueueItem{url, css_url});

        emit parent->fonts_queued(queued.size());
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

glaxnimate::model::FontLoader::FontLoader()
    : d(std::make_unique<Private>())
{
    d->parent = this;
    connect(&d->downloader, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply){
        d->handle_response(reply);
    });
}

glaxnimate::model::FontLoader::~FontLoader()
{
}

void glaxnimate::model::FontLoader::clear()
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

void glaxnimate::model::FontLoader::load_queue()
{
    d->loading = true;
    for ( const auto& url : d->queued )
        d->load_item(url);

    emit fonts_queued(d->queued.size());
    emit fonts_loaded(0);
}

void glaxnimate::model::FontLoader::queue(const QUrl& url)
{
    d->queue({url});
}

int glaxnimate::model::FontLoader::queued_total() const
{
    return d->queued.size();
}

const std::vector<glaxnimate::model::CustomFont> & glaxnimate::model::FontLoader::fonts() const
{
    return d->fonts;
}

void glaxnimate::model::FontLoader::queue_data(const QByteArray& data)
{
    d->parse(data, {}, {});
}


void glaxnimate::model::FontLoader::cancel()
{
    if ( d->loading )
    {
        for ( const auto& reply : d->active_replies )
            reply->abort();
        clear();
    }
}

bool glaxnimate::model::FontLoader::loading() const
{
    return d->loading;
}
