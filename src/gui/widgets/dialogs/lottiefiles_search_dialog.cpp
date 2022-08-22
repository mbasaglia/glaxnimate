#include "lottiefiles_search_dialog.hpp"
#include "ui_lottiefiles_search_dialog.h"
#include <QEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QImageReader>

#include "app/log/log.hpp"

class LottieFilesSearchDialog::Private
{
public:
    struct Result
    {
        int id;
        QString name;
        QUrl url;
        QUrl preview_url;
        QUrl lottie;
        QColor background;
        int likes;
        int comments;
        QImage preview = {};
    };

    void search(const QString& query, int page)
    {
        static QString graphql_query = R"(
            "query Search($withAep: Boolean, $pageSize: Float, $page: Float, $query: String!)
            {
                search(withAep: $withAep, pageSize: $pageSize, page: $page, query: $query)
                {
                    query,
                    currentPage,
                    totalPages,
                    results
                    {
                        bgColor,
                        id,
                        imageFrame,
                        imageUrl,
                        lottieUrl,
                        name,
                        url,
                        likesCount,
                        commentsCount
                    }
                }
            })";
        QJsonObject graphql_dict;
        graphql_dict["query"] = graphql_query;
        QJsonObject vars;
        vars["withAep"] = false;
        vars["pageSize"] = 10;
        vars["page"] = page;
        vars["query"] = query;
        graphql_dict["variables"] = vars;

        current_query = query;
        current_page = page;

        QNetworkRequest req(graphql_url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        start_load();
        auto reply = http.post(req, QJsonDocument(graphql_dict).toJson());

        connect(reply, &QNetworkReply::finished, dialog, [this, reply]{on_response(reply);});
    }

    void on_response(QNetworkReply* reply)
    {
        end_load();

        if ( reply->error() )
        {
            auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            if ( code.isValid() )
            {
                auto msg = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
                on_error(tr("HTTP Error %1: %2").arg(code.toInt()).arg(msg));
            }
            else
            {
                on_error(tr("Network Error"));
            }
            return;
        }

        auto response = QJsonDocument::fromJson(reply->readAll()).object();

        if ( response.contains("errors") )
        {
            for ( const auto& errv : response["errors"].toArray() )
            {
                auto err = errv.toObject();
                auto stream = app::log::Log("graphql").stream(app::log::Warning);
                stream << err["message"].toString();
                if ( err.contains("locations") )
                {
                    for ( const auto& locv : err["locations"].toArray() )
                    {
                        auto loc = locv.toObject();
                        stream << " " << loc["line"].toInt() << ":" << loc["column"].toInt();
                    }
                }
            }
        }

        if ( !response["data"].isObject() )
        {
            on_error("GraphQL Error");
            return;
        }

        auto data = response["data"].toObject();
        current_query = data["query"].toString();
        current_page = data["currentPage"].toInt();
        max_pages = data["totalPages"].toInt();
        results.clear();
        auto qresults = data["results"].toArray();
        results.reserve(qresults.size());
        for ( const auto& rev : qresults )
        {
            auto result = rev.toObject();
            results.push_back({
                result["id"].toInt(),
                result["name"].toString(),
                result["url"].toString(),
                result["imageUrl"].toString(),
                result["lottieUrl"].toString(),
                QColor(result["bgColor"].toString()),
                result["likes"].toInt(),
                result["comments"].toInt(),
            });
            add_result(results.back(), results.size() - 1);
        }

    }

    void on_error(const QString& msg)
    {
        /// TODO
    }

    void start_load()
    {
        // TODO
    }

    void end_load()
    {
        // TODO
    }

    void add_result(const Result& result, std::size_t index)
    {
        // TODO widget

        auto reply = http.get(QNetworkRequest(result.preview_url));

        connect(reply, &QNetworkReply::finished, dialog, [this, reply, index, id=result.id]{
            if ( reply->error() )
                return;

            if ( index < results.size() && results[index].id == id )
            {
                QImageReader reader(reply);
                results[index].preview = reader.read();
                QPixmap pix = QPixmap::fromImage(results[index].preview.scaled(QSize(200, 200), Qt::KeepAspectRatio));
                // TODO
            }
        });


    }

    Ui::LottieFilesSearchDialog ui;
    QNetworkAccessManager http;
    QUrl graphql_url{"https://graphql.lottiefiles.com/"};
    LottieFilesSearchDialog* dialog;
    QString current_query;
    int current_page = -1;
    int max_pages = 0;
    std::vector<Result> results;

};

LottieFilesSearchDialog::LottieFilesSearchDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->dialog = this;
    d->ui.setupUi(this);
}

LottieFilesSearchDialog::~LottieFilesSearchDialog() = default;

void LottieFilesSearchDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}
