/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "emoji_dialog.hpp"

#include <QToolButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QScrollBar>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QScreen>
#include <QScroller>
#include <QGraphicsSimpleTextItem>
#include <QtGlobal>

#include "emoji_data.hpp"
#include "style/scroll_area_event_filter.hpp"
#include "emoji_set.hpp"

class glaxnimate::emoji::EmojiDialog::Private
{
public:
    Private(EmojiDialog* parent) : parent(parent)
    {
        parent->setWindowTitle(tr("Select Emoji"));

        QVBoxLayout* lay = new QVBoxLayout(parent);
        parent->setLayout(lay);

        title = new QHBoxLayout;
        lay->addLayout(title);

        font.setPixelSize(cell_size);
        int pad = 10;

        scene_width = columns * (cell_size + cell_margin) + pad;

        section_font.setBold(true);

        table = new QGraphicsView(parent);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setRenderHint(QPainter::Antialiasing);
        table->setRenderHint(QPainter::SmoothPixmapTransform);
#ifdef Q_OS_ANDROID
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#else
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif
        lay->addWidget(table);
        table->setScene(&scene);


        scroller.set_target(table);
        table->viewport()->installEventFilter(&scroller);
        connect(&scroller, &gui::ScrollAreaEventFilter::clicked, parent,
            [parent, this](const QPoint& pos){
                if ( auto item = scene.itemAt(table->mapToScene(pos), table->viewportTransform()) )
                {
                    for ( auto lab : section_headers )
                        if ( lab == item )
                            return;

                    current_unicode = item->data(0).toString();
                    current_slug = item->data(1).toString();
                    parent->accept();
                }
        });

#ifndef Q_OS_ANDROID
        parent->resize(parent->width(), 1024);
        table->viewport()->setMinimumWidth(scene_width);
        auto margins = table->contentsMargins();
        auto scroll_width = table->verticalScrollBar()->sizeHint().width();
        table->setMinimumWidth(scene_width + scroll_width + margins.left() + margins.right());
#endif

        connect(
            QGuiApplication::primaryScreen(),
            &QScreen::primaryOrientationChanged,
            parent,
            [this]{on_rotate();}
        );

    }

    void on_rotate()
    {
        qreal factor = 1;
        factor = table->viewport()->width() / scene_width;
        factor /= table->transform().m11();
        table->scale(factor, factor);
    }

    QGraphicsItem* create_item(const Emoji& emoji)
    {
        switch ( mode )
        {
            case Text:
            {
                auto item = scene.addSimpleText(emoji.unicode);
                item->setFont(font);

                auto rect = item->boundingRect();

                if ( rect.width() > font.pixelSize() * 1.25 )
                {
                    delete item;
                    return nullptr;
                }

                return item;
            }

            case Image:
            {
                auto name = image_slug.slug(emoji.hex_slug) + image_suffix;
                if ( !image_path.exists(name) )
                    return nullptr;

                QPixmap pix;
                pix.load(image_path.absoluteFilePath(name));
                if ( pix.isNull() || pix.width() == 0)
                    return nullptr;

                QGraphicsPixmapItem* item = new QGraphicsPixmapItem(pix);
                item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
                item->setScale(cell_size / pix.width());
                scene.addItem(item);
                return  item;
            }
        }

        return nullptr;
    }

    void load_step()
    {
        const auto& grp = *EmojiGroup::table[curr_group];


        if ( curr_subgroup == 0 )
        {
            section_headers[curr_group]->setVisible(true);
            section_headers[curr_group]->setPos(0, y);
            y += section_headers[curr_group]->boundingRect().height();
        }

        const auto& sub = grp.children[curr_subgroup];

        for ( const auto& emoji : sub->emoji )
        {
            auto item = create_item(emoji);

            if ( !item )
                continue;

            item->setData(0, emoji.unicode);
            item->setData(1, emoji.hex_slug);
            auto rect = item->boundingRect();
            item->setPos(QPointF(column*(cell_size+cell_margin), y) - rect.topLeft());

            column++;
            if ( column == columns )
            {
                y += cell_size+cell_margin;
                column = 0;
            }
        }

        curr_subgroup++;

        if ( curr_subgroup >= int(grp.children.size()) )
        {
            curr_subgroup = 0;
            curr_group++;

            y += cell_size+cell_margin;
            column = 0;
        }
    }

    void load_start()
    {
        for ( const auto& grp : EmojiGroup::table )
        {
            auto first = grp->children[0]->emoji[0];
            auto group_label = scene.addSimpleText(grp->name);
            section_headers.push_back(group_label);
            group_label->setVisible(false);
            group_label->setFont(font);
            if ( group_label->boundingRect().width() > scene_width )
                group_label->setScale(scene_width / group_label->boundingRect().width());
            QToolButton* btn = new QToolButton(parent);
            if ( mode == Text )
            {
                btn->setText(first.unicode);
            }
            else
            {
                btn->setIcon(QIcon(
                    image_path.absoluteFilePath(image_slug.slug(first) + image_suffix)
                ));
                btn->setIconSize(QSize(cell_size * 0.6, cell_size * 0.6));
            }
            connect(btn, &QAbstractButton::clicked, parent, [this, group_label]{
                scroller.scroll_to(group_label->pos() * table->transform().m11());
            });
            title->addWidget(btn);
        }
    }

    QString current_unicode;
    QString current_slug;
    gui::ScrollAreaEventFilter scroller;
    EmojiDialog* parent;

    QFont font;
    QFont section_font;

    QHBoxLayout* title;
    QGraphicsView* table;
    std::vector<QGraphicsSimpleTextItem*> section_headers;
    QGraphicsScene scene;

    qreal y = 0;
    int column = 0;
    int columns = 8;
    int curr_group = 0;
    int curr_subgroup = 0;
    qreal scene_width;
    qreal cell_size = 72;
    qreal cell_margin = 10;

    DisplayMode mode;
    QDir image_path;
    QString image_suffix = ".png";
    EmojiSetSlugFormat image_slug;
};

glaxnimate::emoji::EmojiDialog::EmojiDialog(QWidget *parent)
    : QDialog(parent), d(std::make_unique<Private>(this))
{
#ifdef Q_OS_ANDROID
    d->cell_margin = 0;
#endif
}

glaxnimate::emoji::EmojiDialog::~EmojiDialog()
{}

QString glaxnimate::emoji::EmojiDialog::current_unicode() const
{
    return d->current_unicode;
}

void glaxnimate::emoji::EmojiDialog::timerEvent(QTimerEvent *event)
{
    d->load_step();
    if ( d->curr_group >= int(EmojiGroup::table.size()) )
        killTimer(event->timerId());
}

void glaxnimate::emoji::EmojiDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    d->on_rotate();
#ifdef Q_OS_ANDROID
    d->table->verticalScrollBar()->setValue(0);
#endif
}

QString glaxnimate::emoji::EmojiDialog::current_slug() const
{
    return d->current_slug;
}

void glaxnimate::emoji::EmojiDialog::load_emoji(glaxnimate::emoji::EmojiDialog::DisplayMode mode)
{
    d->mode = mode;
    d->load_start();
    d->load_step();
    startTimer(20);
}

void glaxnimate::emoji::EmojiDialog::set_emoji_font(const QFont& font)
{
    d->font = font;
}

const QFont & glaxnimate::emoji::EmojiDialog::emoji_font() const
{
    return d->font;
}

void glaxnimate::emoji::EmojiDialog::set_image_path(const QDir& path)
{
    d->image_path = path;
}

const QDir & glaxnimate::emoji::EmojiDialog::image_path() const
{
    return d->image_path;
}

void glaxnimate::emoji::EmojiDialog::set_image_suffix(const QString& suffix)
{
    d->image_suffix = suffix;
}

const QString & glaxnimate::emoji::EmojiDialog::image_suffix() const
{
    return d->image_suffix;
}

void glaxnimate::emoji::EmojiDialog::set_image_slug_format(const glaxnimate::emoji::EmojiSetSlugFormat& slug)
{
    d->image_slug = slug;
}

const glaxnimate::emoji::EmojiSetSlugFormat & glaxnimate::emoji::EmojiDialog::image_slug_format() const
{
    return d->image_slug;
}

void glaxnimate::emoji::EmojiDialog::from_emoji_set(const glaxnimate::emoji::EmojiSet& set, int size)
{
    const auto& path = set.download.paths.at(size);
    set_image_path(set.image_path(size));
    set_image_slug_format(set.slug);
    set_image_suffix("." + path.format);
}

void glaxnimate::emoji::EmojiDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
#ifndef Q_OS_ANDROID
    d->on_rotate();
#endif
}
