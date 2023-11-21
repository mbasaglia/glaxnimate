/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
#include "search_result.hpp"
#include "graphql.hpp"


class glaxnimate::gui::LottieFilesSearchDialog::Private
{
public:
    enum SearchType
    {
        Search,
        Featured
    };

    enum Direction
    {
        NewSearch,
        Next,
        Previous
    };

    struct Pagination
    {
        QString start;
        QString end;

        void next(QJsonObject& vars, int page_size) const
        {
            vars["first"] = page_size;
            vars["after"] = end;
            vars["before"] = QJsonValue::Null;
            vars["last"] = QJsonValue::Null;
        }

        void previous(QJsonObject& vars, int page_size) const
        {
            vars["first"] = QJsonValue::Null;
            vars["after"] = QJsonValue::Null;
            vars["before"] = start;
            vars["last"] = page_size;
        }

        void new_search(QJsonObject& vars, int page_size) const
        {
            vars["first"] = page_size;
            vars["after"] = QJsonValue::Null;
            vars["before"] = QJsonValue::Null;
            vars["last"] = QJsonValue::Null;
        }

        void vars(QJsonObject& vars, int page_size, Direction direction) const
        {
            switch ( direction )
            {
                case NewSearch: return new_search(vars, page_size);
                case Next: return next(vars, page_size);
                case Previous: return previous(vars, page_size);
            }
        }
    };

    void search(SearchType type, Direction direction)
    {
        this->type = type;

        switch ( type )
        {
            case Search:
                return search_query(current_query, direction);
            case Featured:
                return featured(direction);
        }
    }

    void search_query(const QString& query, Direction direction)
    {
        static QString graphql_query = R"(
            query Search($before: String, $after: String, $first: Int, $last: Int, $query: String!)
            {
                searchPublicAnimations(before: $before, after: $after, first: $first, last: $last, query: $query)
                {
                    edges {
                        node {
                            bgColor,
                            id,
                            imageFrame,
                            imageUrl,
                            jsonUrl,
                            name,
                            url,
                            likesCount,
                            commentsCount,
                            createdBy {
                                username
                            }
                        }
                    }
                    pageInfo {
                        startCursor
                        endCursor
                        hasNextPage
                        hasPreviousPage
                    }
                    totalCount
                }
            })";
        QJsonObject vars;
        pagination.vars(vars, layout_rows * layout_columns, direction);
        vars["query"] = query;

        current_query = query;
        graphql.query(graphql_query, vars);
    }

    void featured(Direction direction)
    {
        static QString query = R"(
            query Search($before: String, $after: String, $first: Int, $last: Int, $collectionId: Float!)
            {
                publicCollectionAnimations(before: $before, after: $after, first: $first, last: $last, collectionId: $collectionId)
                {
                    edges {
                        node {
                            bgColor,
                            id,
                            imageFrame,
                            imageUrl,
                            jsonUrl,
                            name,
                            url,
                            likesCount,
                            commentsCount,
                            createdBy {
                                username
                            }
                        }
                    }
                    pageInfo {
                        startCursor
                        endCursor
                        hasNextPage
                        hasPreviousPage
                    }
                    totalCount
                }
            })";
        QJsonObject vars;
        pagination.vars(vars, layout_rows * layout_columns, direction);
        vars["collectionId"] = 1318984.;

        current_query = "";

        graphql.query(query, vars);
    }

    void on_response(QNetworkReply* reply)
    {
        end_load();

        auto response = QJsonDocument::fromJson(reply->readAll()).object();

        if ( response.contains("errors") )
        {
            for ( const auto& errv : response["errors"].toArray() )
            {
                auto err = errv.toObject();
                auto stream = app::log::Log("lottiefiles", "graphql").stream(app::log::Warning);
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

        auto d = response["data"].toObject();

        if ( !response["data"].isObject() || (!d["searchPublicAnimations"].isObject() && !d["publicCollectionAnimations"].isObject()) )
        {
            if ( reply->error() )
            {
                auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                if ( code.isValid() )
                {
                    auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
                    auto msg = tr("HTTP Error %1: %2").arg(code.toInt()).arg(reason);
                    app::log::Log("lottiefiles", "http").stream(app::log::Warning) << reply->url().toString() << msg;
                    on_error(msg);
                }
                else
                {
                    on_error(tr("Network Error"));
                }
            }
            else
            {
                on_error("GraphQL Error");
            }
            return;
        }

        QJsonObject data;
        if ( d["publicCollectionAnimations"].isObject() )
            data = d["publicCollectionAnimations"].toObject();
        else
            data = d["searchPublicAnimations"].toObject();

        clear_results();
        auto qresults = data["edges"].toArray();
        results.reserve(qresults.size());
        for ( const auto& rev : qresults )
        {
            auto result = rev.toObject()["node"].toObject();
            add_result({
                result["id"].toInt(),
                result["name"].toString(),
                result["createdBy"].toObject()["username"].toString(),
                QUrl(result["url"].toString()),
                QUrl(result["imageUrl"].toString()),
                QUrl(result["jsonUrl"].toString()),
                QColor(result["bgColor"].toString()),
                result["likesCount"].toInt(),
                result["commentsCount"].toInt(),
            });
        }

        auto page_info = data["pageInfo"].toObject();

        pagination.end = page_info["endCursor"].toString();
        pagination.start = page_info["startCursor"].toString();

        ui.button_next->setEnabled(page_info["hasNextPage"].toBool());
        ui.button_previous->setEnabled(page_info["hasPreviousPage"].toBool());
    }

    void on_error(const QString& msg)
    {
        ui.label_error->setVisible(true);
        ui.label_error->setText(msg);
    }

    void start_load()
    {
        ui.label_error->setVisible(false);
        ui.progress_bar->setMaximum(100);
        ui.progress_bar->setValue(0);
        ui.progress_bar->setVisible(true);
        ui.button_next->setEnabled(false);
    }

    void end_load()
    {
        ui.label_error->setVisible(false);
        ui.progress_bar->setVisible(false);
    }

    void clear_results()
    {
        ui.button_import->setEnabled(false);
        ui.button_open->setEnabled(false);
        results.clear();
    }

    void add_result(LottieFilesResult result)
    {
        std::size_t index = results.size();
        results.emplace_back(std::make_unique<LottieFilesResultItem>(std::move(result), dialog));
        auto widget = results.back().get();
        int row = index / layout_columns;
        int col = index % layout_columns;
        ui.result_area_layout->addWidget(widget, row, col);
        connect(widget, &LottieFilesResultItem::selected, dialog, [this](const QString& name, const QUrl& url) {
            on_selected(name, url);
        });
        connect(widget, &LottieFilesResultItem::selected_open, dialog, [this](const QString& name, const QUrl& url) {
            on_selected(name, url);
            dialog->done(Open);
        });
        connect(widget, &LottieFilesResultItem::selected_import, dialog, [this](const QString& name, const QUrl& url) {
            on_selected(name, url);
            dialog->done(Import);
        });

        auto reply = graphql.http().get(QNetworkRequest(widget->result().preview_url));
        connect(reply, &QNetworkReply::finished, widget, [widget, reply]{
            if ( reply->error() )
                return;

            widget->set_preview_image(QImageReader(reply).read());
        });
    }

    void on_selected(const QString& name, const QUrl& url)
    {
        current_name = name;
        current_url = url;
        ui.button_import->setEnabled(true);
        ui.button_open->setEnabled(true);
    }

    void on_progress(quint64 bytes, quint64 total)
    {
        static constexpr const int maxi = std::numeric_limits<int>::max();
        if ( total > maxi )
        {
            bytes = (long double)(maxi) / total * bytes;
            total = maxi;
        }
        ui.progress_bar->setMaximum(total);
        ui.progress_bar->setValue(bytes);
    }

    Ui::LottieFilesSearchDialog ui;
    GraphQl graphql{"https://graphql.lottiefiles.com/2022-08/"};
    LottieFilesSearchDialog* dialog;
    QString current_query;
    std::vector<std::unique_ptr<LottieFilesResultItem>> results;
    int layout_columns = 4;
    int layout_rows = 3;
    QUrl current_url;
    QString current_name;
    Pagination pagination;
    SearchType type = SearchType::Featured;
};

glaxnimate::gui::LottieFilesSearchDialog::LottieFilesSearchDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->dialog = this;
    d->ui.setupUi(this);
    d->ui.progress_bar->setVisible(false);
    d->clear_results();
    d->ui.result_area->setMinimumWidth(80 * d->layout_columns + 128);
    d->featured(Private::NewSearch);

    connect(&d->graphql, &GraphQl::query_started, this, [this]{d->start_load();});
    connect(&d->graphql, &GraphQl::query_finished, this, [this](QNetworkReply* reply){d->on_response(reply);});
    connect(&d->graphql, &GraphQl::query_progress, this, [this](quint64 bytes, quint64 total){d->on_progress(bytes, total);});
}

glaxnimate::gui::LottieFilesSearchDialog::~LottieFilesSearchDialog() = default;

void glaxnimate::gui::LottieFilesSearchDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_import()
{
    done(Import);
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_open()
{
    done(Open);
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_search()
{
    if ( d->current_query != d->ui.input_query->text() )
    {
        d->current_query = d->ui.input_query->text();
        d->search(d->current_query.isEmpty() ? Private::Featured : Private::Search, Private::NewSearch);
    }
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_next()
{
    d->search(d->type, Private::Next);
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_previous()
{
    d->search(d->type, Private::Previous);
}

const QUrl & glaxnimate::gui::LottieFilesSearchDialog::selected_url() const
{
    return d->current_url;
}

const QString & glaxnimate::gui::LottieFilesSearchDialog::selected_name() const
{
    return d->current_name;
}
