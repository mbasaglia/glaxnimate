#include "google_fonts_model.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "app/settings/settings.hpp"


class glaxnimate::gui::font::GoogleFontsModel::Private
{
public:
    GoogleFontsModel* parent;
    QString url_base;
    QString token;
    std::vector<GoogleFont> fonts;
//     std::unordered_map<QString, std::size_t> font_names;
    QNetworkAccessManager downloader;
    std::unordered_map<QString, std::map<std::pair<int, bool>, int>> downloaded;

    bool response_has_error(QNetworkReply* reply)
    {
        emit parent->max_progress_changed(0);

        if ( reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 )
        {
            auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            emit parent->error(reason);
            return true;
        }

        return false;
    }

    void style_from_slug(const QString& slug, GoogleFont::Style& out )
    {
        out.italic = slug.endsWith("italic");
        if ( slug[2] == '0' )
            out.weight = slug.left(3).toInt();
    }

    void parse_json(const QJsonDocument& doc)
    {
        emit parent->beginResetModel();

        int pop = 0;
        auto items = doc.object()["items"].toArray();
//         font_names.clear();
        fonts.clear();
        fonts.reserve(items.size());
        for ( QJsonValue font_json : items )
        {
            GoogleFont font;
            font.family = font_json["family"].toString();
            font.popularity_index = ++pop;

            std::map<std::pair<int, bool>, int>* downloaded_font = nullptr;
            auto downloaded_font_iter = downloaded.find(font.family);
            if ( downloaded_font_iter != downloaded.end() )
                downloaded_font = &downloaded_font_iter->second;

            QString cat = font_json["category"].toString();
            if ( cat == "sans-serif" )
                font.category = GoogleFont::SansSerif;
            else if ( cat == "serif" )
                font.category = GoogleFont::Serif;
            else if ( cat == "display" )
                font.category = GoogleFont::Display;
            else if ( cat == "handwriting" )
                font.category = GoogleFont::Handwriting;
            else if ( cat == "monospace" )
                font.category = GoogleFont::Monospace;

            auto items = font_json["files"].toObject();
            for ( auto it = items.begin(); it != items.end(); ++it )
            {
                GoogleFont::Style& style = font.styles.emplace_back();
                style.url = it->toString();
                style_from_slug(it.key(), style);

                if ( downloaded_font )
                {
                    auto downloaded_style_iter = downloaded_font->find({style.weight, style.italic});
                    if ( downloaded_style_iter != downloaded_font->end() )
                    {
                        int db_index = downloaded_style_iter->second;
                        style.font_database_index = db_index;
                        style.font_database_family = QFontDatabase::applicationFontFamilies(db_index)[0];
                    }
                }
            }

            std::sort(font.styles.begin(), font.styles.end());

            if ( !font.styles.empty() )
            {
//                 font_names[font.family] = fonts.size();
                fonts.push_back(font);
            }
        }

        emit parent->endResetModel();
    }

    void update_settings()
    {
        token = app::settings::get<QString>("api_credentials", "Google Fonts/Token");
        url_base = app::settings::get<QString>("api_credentials", "Google Fonts/URL");
    }

    void font_changed(std::size_t font_index)
    {
        emit parent->dataChanged(parent->createIndex(font_index, 0), parent->createIndex(font_index, Column::Count - 1));
    }

    void download_style(GoogleFont* font, std::size_t font_index, int style_index, GoogleFont::StyleList::iterator style)
    {
        if ( style == font->styles.end() )
        {
            if ( font->status == GoogleFont::InProgress )
            {
                font->status = GoogleFont::Downloaded;
                font_changed(font_index);
                emit parent->download_finished(font_index);
            }
            return;
        }

        QNetworkRequest request(style->url);
        request.setMaximumRedirectsAllowed(3);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

        auto reply = downloader.get(request);

        int base = style_index * 100;
        connect(reply, &QNetworkReply::downloadProgress, parent, [this, base](qint64 received, qint64 total){
            emit parent->progress_changed(received * 100 / total + base);
        });
        connect(reply, &QNetworkReply::finished, parent, [this, reply, base, font, font_index, style, style_index]{

            emit parent->progress_changed(base + 100);

            if ( response_has_error(reply) )
            {

                font->status = GoogleFont::Broken;
                font_changed(font_index);
            }
            else
            {
                auto data = reply->readAll();
                reply->close();
                int db_index = QFontDatabase::addApplicationFontFromData(data);
                style->font_database_index = db_index;
                if ( db_index == -1 )
                {
                    font->status = GoogleFont::Broken;
                    font_changed(font_index);
                    emit parent->error(tr("Could not add font"));
                }
                else
                {
                    downloaded[font->family][{style->weight, style->italic}] = db_index;
                    style->font_database_family = QFontDatabase::applicationFontFamilies(db_index)[0];
                }
            }

            auto next = style;
            ++next;
            download_style(font, font_index, style_index+1, next);
        });
    }

    void download_font(GoogleFont& font, std::size_t font_index)
    {
        font.status = GoogleFont::InProgress;
        font_changed(font_index);
        emit parent->progress_changed(0);
        emit parent->max_progress_changed(100 * font.styles.size());
        download_style(&font, font_index, 0, font.styles.begin());
    }
};

glaxnimate::gui::font::GoogleFontsModel::GoogleFontsModel()
    : d(std::make_unique<Private>())
{
    d->parent = this;
}

glaxnimate::gui::font::GoogleFontsModel::~GoogleFontsModel()
{
}

void glaxnimate::gui::font::GoogleFontsModel::response_progress(qint64 received, qint64 total)
{
    emit max_progress_changed(total);
    emit progress_changed(received);
}


void glaxnimate::gui::font::GoogleFontsModel::refresh()
{
    d->update_settings();
    QNetworkRequest request(QUrl(d->url_base + "?sort=popularity&key=" + d->token));
    request.setMaximumRedirectsAllowed(3);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    emit max_progress_changed(100);
    emit progress_changed(0);

    auto reply = d->downloader.get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, &GoogleFontsModel::response_progress);
    connect(reply, &QNetworkReply::finished, this, [this, reply]{
        if ( d->response_has_error(reply) )
            return;

        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(reply->readAll(), &error);
        reply->close();

        if ( error.error )
            emit this->error(error.errorString());

        d->parse_json(doc);
    });
}

int glaxnimate::gui::font::GoogleFontsModel::columnCount(const QModelIndex&) const
{
    return Column::Count;
}

int glaxnimate::gui::font::GoogleFontsModel::rowCount(const QModelIndex&) const
{
    return d->fonts.size();
}

Qt::ItemFlags glaxnimate::gui::font::GoogleFontsModel::flags(const QModelIndex& index) const
{
    return QAbstractTableModel::flags(index);
}

QVariant glaxnimate::gui::font::GoogleFontsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Vertical )
        return {};

    if ( role != Qt::DisplayRole )
        return {};

    switch ( section )
    {
        case Column::Family:
            return tr("Family");
        case Column::Category:
            return tr("Category");
        case Column::Popularity:
            return tr("Rank");
        case Column::Status:
            return tr("Status");
    }

    return {};
}

QVariant glaxnimate::gui::font::GoogleFontsModel::data(const QModelIndex& index, int role) const
{
    if ( !index.isValid() )
        return {};

    int row = index.row();
    if ( row < 0 || row >= int(d->fonts.size()) )
        return {};

    const auto& font = d->fonts[row];

    switch ( index.column() )
    {
        case Column::Family:
            if ( role == Qt::DisplayRole || role == SortRole )
                return font.family;
            break;

        case Column::Category:
            if ( role == Qt::DisplayRole )
            {
                switch ( font.category )
                {
                    case GoogleFont::SansSerif:
                        return tr("Sans-Serif");
                    case GoogleFont::Serif:
                        return tr("Serif");
                    case GoogleFont::Monospace:
                        return tr("Monospace");
                    case GoogleFont::Display:
                        return tr("Display");
                    case GoogleFont::Handwriting:
                        return tr("Handwriting");
                }
            }
            break;

        case Column::Popularity:
            if ( role == Qt::DisplayRole || role == SortRole )
                return font.popularity_index;
            break;

        case Column::Status:
            if ( role == Qt::DecorationRole )
            {
                switch ( font.status )
                {
                    case GoogleFont::Downloaded:
                        return QIcon::fromTheme("package-installed-updated");
                    case GoogleFont::Available:
                        return QIcon::fromTheme("package-available");
                    case GoogleFont::InProgress:
                        return QIcon::fromTheme("package-install");
                    case GoogleFont::Broken:
                        return QIcon::fromTheme("package-broken");
                }
            }
            else if ( role == SortRole )
            {
                return font.status;
            }
            break;
    }

    return {};
}

bool glaxnimate::gui::font::GoogleFontsModel::has_token() const
{
    d->update_settings();
    return !d->token.isEmpty();
}

void glaxnimate::gui::font::GoogleFontsModel::download_font(int row)
{
    if ( row < 0 || row >= int(d->fonts.size()) )
        return ;

    d->download_font(d->fonts[row], row);
}

/*
void glaxnimate::gui::font::GoogleFontsModel::download_font(const QString& family)
{
    auto it = d->font_names.find(family);
    if ( it == d->font_names.end() )
        return;

    auto& font = d->fonts[it->second];
    d->download_font(font, it->second);
}

const glaxnimate::gui::font::GoogleFontsModel::GoogleFont* glaxnimate::gui::font::GoogleFontsModel::font(const QString& family) const
{
    auto it = d->font_names.find(family);
    if ( it == d->font_names.end() )
        return nullptr;

    return &d->fonts[it->second];
}
*/
const glaxnimate::gui::font::GoogleFontsModel::GoogleFont* glaxnimate::gui::font::GoogleFontsModel::font(int row) const
{
    if ( row < 0 || row >= int(d->fonts.size()) )
        return nullptr;

    return &d->fonts[row];
}


QString glaxnimate::gui::font::GoogleFontsModel::style_name(const GoogleFont::Style& style) const
{
    QString italic = style.italic ? tr("Italic") : "";

    if ( style.weight == 0 && style.italic )
        return italic;

    QString weight_name;
    switch ( style.weight )
    {
        case 100: weight_name = tr("Thin"); break;
        case 200: weight_name = tr("Extra-Light"); break;
        case 300: weight_name = tr("Light"); break;
        case 400: weight_name = tr("Regular"); break;
        case 500: weight_name = tr("Medium"); break;
        case 600: weight_name = tr("Semi-Bold"); break;
        case 700: weight_name = tr("Bold"); break;
        case 800: weight_name = tr("Extra-Bold"); break;
        case 900: weight_name = tr("Black"); break;
    }

    //: %1 is the weight (Bold), %2 is "Italic" when needed
    return tr("%1 %2").arg(weight_name).arg(italic).trimmed();
}
