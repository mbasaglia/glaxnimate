#include "graphql.hpp"


void glaxnimate::gui::GraphQl::query(const QString& query, const QJsonObject& vars)
{
    QJsonObject graphql_dict;
    graphql_dict["query"] = query;
    graphql_dict["variables"] = vars;


    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    emit query_started();
    auto reply = http_.post(req, QJsonDocument(graphql_dict).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]{ emit this->query_finished(reply); });
    connect(reply, &QNetworkReply::downloadProgress, this, &GraphQl::query_progress);
}
