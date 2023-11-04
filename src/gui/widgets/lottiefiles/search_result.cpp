/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "search_result.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QIcon>
#include <QPushButton>
#include <QToolButton>
#include <QDesktopServices>
#include <QPainter>

glaxnimate::gui::LottieFilesResultItem::LottieFilesResultItem(LottieFilesResult res, QWidget* parent)
: QWidget(parent), data(std::move(res))
{
    QVBoxLayout* lay = new QVBoxLayout(this);

    QLabel* name = new QLabel(data.name, this);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QFont name_font = name->font();
    name_font.setBold(true);
    name->setFont(name_font);
    lay->addWidget(name);

    QLabel* by = new QLabel(tr("by %1").arg(data.author_username), this);
    by->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    by->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QFont by_font = by->font();
    by_font.setPointSizeF(by_font.pointSizeF() * 0.6);
    by->setFont(by_font);
    lay->addWidget(by);

    spacer = new QSpacerItem(image_size.width(), image_size.height(), QSizePolicy::Preferred, QSizePolicy::Preferred);
    lay->addItem(spacer);

    auto stats = new QHBoxLayout();
    lay->addLayout(stats);

    auto likes_icon = new QLabel(this);
    likes_icon->setPixmap(QIcon::fromTheme("emblem-favorite-symbolic").pixmap(32));
    likes_icon->setToolTip(tr("Likes"));
    stats->addWidget(likes_icon);
    auto likes_num = new QLabel(QString::number(data.likes), this);
    stats->addWidget(likes_num);

    stats->addSpacerItem(new QSpacerItem(1, 0, QSizePolicy::Expanding));

    auto comments_icon = new QLabel(this);
    comments_icon->setPixmap(QIcon::fromTheme("comment-symbolic").pixmap(32));
    comments_icon->setToolTip(tr("Comments"));
    stats->addWidget(comments_icon);
    auto comments_num = new QLabel(QString::number(data.comments), this);
    stats->addWidget(comments_num);

    auto buttons = new QHBoxLayout();
    lay->addLayout(buttons);

    auto view = new QToolButton(this);
    view->setText(tr("View on LottieFiles..."));
    view->setToolTip(view->text());
    view->setIcon(QIcon::fromTheme("internet-web-browser"));
    buttons->addWidget(view);
    connect(view, &QToolButton::clicked, this, [this]{QDesktopServices::openUrl(data.url);});

    auto open = new QToolButton(this);
    open->setText(tr("Open"));
    open->setToolTip(open->text());
    open->setIcon(QIcon::fromTheme("document-open"));
    buttons->addWidget(open);
    connect(open, &QToolButton::clicked, this, [this]{emit selected_open(data.name, data.lottie);});

    auto import = new QToolButton(this);
    import->setText(tr("Import"));
    import->setToolTip(import->text());
    import->setIcon(QIcon::fromTheme("document-import"));
    buttons->addWidget(import);
    connect(import, &QToolButton::clicked, this, [this]{emit selected_import(data.name, data.lottie);});

    preview = QIcon::fromTheme("application-x-partial-download").pixmap(image_size).toImage();

    setFocusPolicy(Qt::ClickFocus);
}

void glaxnimate::gui::LottieFilesResultItem::set_image_size(const QSize& size)
{
    image_size = size;
    spacer->changeSize(size.width(), size.height(), QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void glaxnimate::gui::LottieFilesResultItem::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    emit selected(data.name, data.lottie);
}

void glaxnimate::gui::LottieFilesResultItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);

    emit selected_import(data.name, data.lottie);
}

void glaxnimate::gui::LottieFilesResultItem::paintEvent(QPaintEvent*)
{
    auto bg = hasFocus() ? this->palette().highlight() : QBrush(Qt::NoBrush);
    QPainter painter(this);
    painter.fillRect(QRect(QPoint(0, 0), size()), bg);

    QRectF area = spacer->geometry();

    if ( preview.width() == 0 || preview.height() == 0 || area.width() == 0 || area.height() == 0 )
        return;

    qreal scalex = qreal(area.width()) / preview.width();
    qreal scaley = qreal(area.height()) / preview.height();
    qreal scale = qMin(scalex, scaley);
    QSizeF target_size{
        preview.width() * scale,
        preview.height() * scale
    };
    QRectF target(
        QPointF{
            area.center().x() - target_size.width() / 2,
            area.center().y() - target_size.height() / 2,
        },
        target_size
    );
    painter.drawImage(target, preview);
}

