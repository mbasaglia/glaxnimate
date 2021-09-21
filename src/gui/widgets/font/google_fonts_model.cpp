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
    QNetworkAccessManager downloader;

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

    void parse_json(const QJsonDocument& doc)
    {
        emit parent->beginResetModel();

        int pop = 0;
        auto items = doc.object()["items"].toArray();
        fonts.clear();
        fonts.reserve(items.size());
        for ( QJsonValue font_json : items )
        {
            GoogleFont font;
            font.family = font_json["family"].toString();
            font.popularity_index = ++pop;

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
                font.variants[it.key()].url = it->toString();

            if ( !font.variants.empty() )
                fonts.push_back(font);
        }

        emit parent->endResetModel();
    }

    void update_settings()
    {
        token = app::settings::get<QString>("api_credentials", "Google Fonts/Token");
        url_base = app::settings::get<QString>("api_credentials", "Google Fonts/URL");
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
    emit progress_changed(total);
    emit max_progress_changed(received);
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
            if ( role == Qt::DisplayRole )
                return font.family;
            break;

        case Column::Category:
            if ( role == Qt::DisplayRole )
            {
                switch ( font.category )
                {
                    case GoogleFont::SansSerif:
                        return tr("Sand-Serif");
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
            if ( role == Qt::DisplayRole )
                return font.popularity_index;
            break;

        case Column::Status:
            if ( role == Qt::DecorationRole )
            {
                for ( const auto& var : font.variants )
                {
                    if ( var.second.font.isValid() )
                        return QIcon::fromTheme("package-installed-updated");
                }
                return QIcon::fromTheme("package-available");
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
