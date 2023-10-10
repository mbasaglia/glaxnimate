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
    void search(const QString& query, const QString& page)
    {
        if ( query.isEmpty() )
            featured(page);
        else
            search_query(query, page);
    }

    void search_query(const QString& query, const QString& page)
    {
        static QString graphql_query = R"(
            query Search($withAep: Boolean, $pageSize: Int, $page: String, $query: String!)
            {
                searchPublicAnimations(withAep: $withAep, first: $pageSize, after: $page, query: $query)
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
                        endCursor
                        hasNextPage
                    }
                    totalCount
                }
            })";
        QJsonObject vars;
        vars["withAep"] = false;
        vars["pageSize"] = layout_rows * layout_columns;
        vars["page"] = page;
        vars["query"] = query;

        current_query = query;
        previous_page = current_page;
        current_page = page;

        graphql.query(graphql_query, vars);
    }

    void featured(const QString& page)
    {
        static QString query = R"(
            query Search($page: String, $pageSize: Int, $collectionId: Float!)
            {
                publicCollectionAnimations(after: $page, first: $pageSize, collectionId: $collectionId)
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
                        endCursor
                        hasNextPage
                    }
                    totalCount
                }
            })";
        QJsonObject vars;
        vars["page"] = page;
        vars["pageSize"] = layout_rows * layout_columns;
        vars["collectionId"] = 1318984.;

        current_query = "";
        previous_page = current_page;
        current_page = page;

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
                result["url"].toString(),
                result["imageUrl"].toString(),
                result["jsonUrl"].toString(),
                QColor(result["bgColor"].toString()),
                result["likesCount"].toInt(),
                result["commentsCount"].toInt(),
            });
        }

        auto page_info = data["pageInfo"].toObject();

        next_page = page_info["endCursor"].toString();
        if ( !page_info["hasNextPage"].toBool() )
            next_page = "";

        ui.button_next->setEnabled(!next_page.isEmpty());
        ui.button_previous->setEnabled(!current_page.isEmpty());
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
    GraphQl graphql{"https://graphql.lottiefiles.com/"};
    LottieFilesSearchDialog* dialog;
    QString current_query;
    std::vector<std::unique_ptr<LottieFilesResultItem>> results;
    QSize preview_size{200, 200};
    int layout_columns = 4;
    int layout_rows = 3;
    QUrl current_url;
    QString current_name;
    QString previous_page;
    QString current_page;
    QString next_page;
};

glaxnimate::gui::LottieFilesSearchDialog::LottieFilesSearchDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->dialog = this;
    d->ui.setupUi(this);
    d->ui.progress_bar->setVisible(false);
    d->clear_results();
    d->ui.result_area->setMinimumWidth(d->preview_size.width() * d->layout_columns + 128);
    d->featured("");

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
    if ( d->current_query != d->ui.input_query->text() && d->ui.input_query->text().size() > 2 )
        d->search(d->ui.input_query->text(), "");
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_next()
{
    d->search(d->current_query, d->next_page);
}

void glaxnimate::gui::LottieFilesSearchDialog::clicked_previous()
{
    d->search(d->current_query, d->previous_page);
}

const QUrl & glaxnimate::gui::LottieFilesSearchDialog::selected_url() const
{
    return d->current_url;
}

const QString & glaxnimate::gui::LottieFilesSearchDialog::selected_name() const
{
    return d->current_name;
}
