/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QString>
#include <QUrl>
#include <QColor>
#include <QImage>
#include <QWidget>
#include <QSpacerItem>

namespace glaxnimate::gui {

class LottieFilesResultItem;

struct LottieFilesResult
{
    int id;
    QString name;
    QString author_username;
    QUrl url;
    QUrl preview_url;
    QUrl lottie;
    QColor background;
    int likes;
    int comments;
};


class LottieFilesResultItem : public QWidget
{
    Q_OBJECT

public:
    LottieFilesResultItem(LottieFilesResult res, QWidget* parent);

    void set_preview_image(QImage preview)
    {
        this->preview = std::move(preview);
        update();
    }

    void set_image_size(const QSize& size);

    const LottieFilesResult& result() const { return data; }

Q_SIGNALS:
    void selected(const QString& name, const QUrl& url);
    void selected_open(const QString& name, const QUrl& url);
    void selected_import(const QString& name, const QUrl& url);

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent * event) override;

private:
    LottieFilesResult data;
    QImage preview = {};
    QSize image_size{128, 128};
    QSpacerItem* spacer;
};

} // namespace glaxnimate::gui
