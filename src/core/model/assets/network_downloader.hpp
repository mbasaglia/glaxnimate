/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <unordered_map>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


namespace glaxnimate::model {

class NetworkDownloader: QObject
{
    Q_OBJECT

private:
    struct PendingRequest
    {
        PendingRequest(QNetworkReply* reply) : reply(reply) {}
        PendingRequest(const PendingRequest&) = delete;
        PendingRequest& operator=(const PendingRequest&) = delete;
        PendingRequest(PendingRequest&& oth)
            : reply(oth.reply)
        {
            oth.reply = nullptr;
        }

        PendingRequest& operator=(PendingRequest&& oth)
        {
            std::swap(reply, oth.reply);
            return *this;
        }

        ~PendingRequest()
        {
            if ( reply )
            {
                aborted = true;
                if ( reply->isRunning() )
                    reply->abort();
                reply->deleteLater();
            }
        }

        QNetworkReply* reply = nullptr;
        qint64 received = 0;
        qint64 total = 0;
        bool aborted = false;
    };

public:
    template<class Func>
    void get(const QUrl& url, const Func& callback, QObject* receiver = nullptr)
    {
        auto reply = manager.get(QNetworkRequest(url));
        pending.insert({reply, PendingRequest(reply)});
        connect(reply, &QNetworkReply::downloadProgress, this, &NetworkDownloader::on_download_progress);
        connect(reply, &QNetworkReply::finished, receiver ? receiver : this, [this, reply, callback]{
            if ( !reply->error() )
                callback(reply->readAll());

            auto it = pending.find(reply);
            if ( it != pending.end() && !it->second.aborted )
            {
                total -= it->second.total;
                received -= it->second.received;
                pending.erase(it);
                if ( pending.empty() )
                    emit download_finished();
            }
        });
    }

private slots:
    void on_download_progress(qint64 bytes_received, qint64 bytes_total)
    {
        if ( bytes_total == -1 )
            bytes_total = 0;

        QObject* request = sender();
        auto it = pending.find(request);
        if ( it == pending.end() )
            return;

        if ( bytes_total != it->second.total )
        {
            total += bytes_total - it->second.total;
            it->second.total = bytes_total;
        }

        it->second.received = bytes_received;

        received += bytes_received;
        if ( bytes_total > 0 )
            emit download_progress(received, total);
    }

signals:
    void download_progress(qint64 bytes_received, qint64 bytes_total);
    void download_finished();

private:
    QNetworkAccessManager manager;
    std::unordered_map<QObject*, PendingRequest> pending;
    qint64 total = 0;
    qint64 received = 0;
};

} // namespace glaxnimate::model
