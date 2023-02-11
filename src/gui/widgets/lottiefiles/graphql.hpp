#pragma once

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace glaxnimate::gui {

class GraphQl: public QObject
{
    Q_OBJECT
public:
    explicit GraphQl(const QUrl& url)
        : url(url)
    {
    }

    explicit GraphQl(const QString& url)
        : GraphQl(QUrl(url))
    {
    }

    void query(const QString& query, const QJsonObject& vars);

    QNetworkAccessManager& http() { return http_; }

signals:
    void query_started();
    void query_progress(quint64 bytes, quint64 total);
    void query_finished(QNetworkReply* reply);

private:
    QUrl url{"https://graphql.lottiefiles.com/"};
    QNetworkAccessManager http_;
};

} // namespace glaxnimate::gui
