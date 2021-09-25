#include "google_fonts_model.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "app/settings/settings.hpp"


QString glaxnimate::gui::font::GoogleFontsModel::GoogleFont::css_url(const Style& style) const
{
    QString fam = QUrl::toPercentEncoding(family);
    static QString base("https://fonts.googleapis.com/css2?family=%1:ital,wght@%2,%3&display=swap");
    return  base.arg(fam).arg(style.italic ? "1" : "0").arg(style.weight);
}


class glaxnimate::gui::font::GoogleFontsModel::Private
{
public:
    GoogleFontsModel* parent;
    QString url_base;
    QString token;
    std::vector<GoogleFont> fonts;
//     std::unordered_map<QString, std::size_t> font_names;
    QNetworkAccessManager downloader;
    std::unordered_map<QString, std::map<std::pair<int, bool>, model::CustomFont>> downloaded;
    std::set<QString> subsets;

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

            std::map<std::pair<int, bool>, model::CustomFont>* downloaded_font = nullptr;
            auto downloaded_font_iter = downloaded.find(font.family);
            if ( downloaded_font_iter != downloaded.end() )
                downloaded_font = &downloaded_font_iter->second;

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
                        style.font = downloaded_style_iter->second;
                }
            }

            std::sort(font.styles.begin(), font.styles.end());

            if ( !font.styles.empty() )
            {
                for ( const auto& val : font_json["subsets"].toArray() )
                {
                    auto subset = val.toString();
                    font.subsets.insert(subset);
                    subsets.insert(subset);
                }

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

                fonts.push_back(font);
            }
        }

        emit parent->endResetModel();
        emit parent->refresh_finished();
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
                style->font = model::CustomFontDatabase::instance().add_font(data);
                if ( !style->font.is_valid() )
                {
                    font->status = GoogleFont::Broken;
                    font_changed(font_index);
                    emit parent->error(tr("Could not add font"));
                }
                else
                {
                    downloaded[font->family][{style->weight, style->italic}] = style->font;
                    style->font.set_source_url(style->url.toString());
                    style->font.set_css_url(font->css_url(*style));
                }
            }

            download_style(font, font_index, style_index+1, style+1);
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
                return category_name(font.category);
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

const glaxnimate::gui::font::GoogleFontsModel::GoogleFont* glaxnimate::gui::font::GoogleFontsModel::font(int row) const
{
    if ( row < 0 || row >= int(d->fonts.size()) )
        return nullptr;

    return &d->fonts[row];
}

bool glaxnimate::gui::font::GoogleFontsModel::has_subset(const QModelIndex& index, const QString& subset) const
{
    if ( !index.isValid() )
        return false;

    int row = index.row();
    if ( row < 0 || row >= int(d->fonts.size()) )
        return false;

    return d->fonts[row].subsets.count(subset);
}

bool glaxnimate::gui::font::GoogleFontsModel::has_category(const QModelIndex& index, GoogleFont::Category cat) const
{
    if ( !index.isValid() )
        return false;

    int row = index.row();
    if ( row < 0 || row >= int(d->fonts.size()) )
        return false;

    return d->fonts[row].category == cat;
}

QString glaxnimate::gui::font::GoogleFontsModel::category_name(GoogleFont::Category category)
{
    switch ( category )
    {
        default:
            return tr("Any");
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

const std::set<QString> & glaxnimate::gui::font::GoogleFontsModel::subsets() const
{
    return d->subsets;
}
