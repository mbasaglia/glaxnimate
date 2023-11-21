/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "graphql.hpp"


void glaxnimate::gui::GraphQl::query(const QString& query, const QJsonObject& vars)
{
    QJsonObject graphql_dict;
    graphql_dict["query"] = query;
    graphql_dict["variables"] = vars;


    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    Q_EMIT query_started();
    auto reply = http_.post(req, QJsonDocument(graphql_dict).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]{ Q_EMIT this->query_finished(reply); });
    connect(reply, &QNetworkReply::downloadProgress, this, &GraphQl::query_progress);
}
