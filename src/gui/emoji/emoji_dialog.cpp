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

#include "emoji_data.hpp"
#include "style/scroll_area_event_filter.hpp"

class glaxnimate::emoji::EmojiDialog::Private
{
public:
    Private(EmojiDialog* parent) : parent(parent)
    {
        QVBoxLayout* lay = new QVBoxLayout(parent);
        parent->setLayout(lay);

        title = new QHBoxLayout;
        lay->addLayout(title);

        font.setPixelSize(cell_size);
        int pad = 10;

        scene_width = columns * cell_size + pad;

        section_font.setBold(true);

        table = new QGraphicsView(parent);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

        for ( const auto& grp : EmojiGroup::table )
        {
            auto first = grp.children[0].emoji[0];
            auto group_label = scene.addSimpleText(grp.name);
            section_headers.push_back(group_label);
            group_label->setVisible(false);
            group_label->setFont(font);
            if ( group_label->boundingRect().width() > scene_width )
                group_label->setScale(scene_width / group_label->boundingRect().width());
            QToolButton* btn = new QToolButton(parent);
            btn->setText(first.unicode);
            connect(btn, &QAbstractButton::clicked, parent, [this, group_label]{
                scroller.scroll_to(group_label->pos() * table->transform().m11());
            });
            title->addWidget(btn);
        }


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
                QPixmap pix;
                auto name = emoji.hex_slug + image_suffix;
                if ( image_path.exists(name) )
                {
                    pix.load(image_path.absoluteFilePath(name));
                }
                else
                {
                    name = emoji.hex_slug.toUpper() + image_suffix;
                    if ( image_path.exists(name) )
                        pix.load(image_path.absoluteFilePath(name));
                }
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
        const auto& grp = EmojiGroup::table[curr_group];


        if ( curr_subgroup == 0 )
        {
            section_headers[curr_group]->setVisible(true);
            section_headers[curr_group]->setPos(0, row*cell_size);
            row++;
        }

        const auto& sub = grp.children[curr_subgroup];

        for ( const auto& emoji : sub.emoji )
        {
            auto item = create_item(emoji);

            if ( !item )
                continue;

            auto rect = item->boundingRect();
            item->setPos(QPointF(column*cell_size, row*cell_size) - rect.topLeft());

            column++;
            if ( column == columns )
            {
                row++;
                column = 0;
            }
        }

        curr_subgroup++;

        if ( curr_subgroup >= int(grp.children.size()) )
        {
            curr_subgroup = 0;
            curr_group++;

            row++;
            column = 0;
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

    int row = 0;
    int column = 0;
    int columns = 8;
    int curr_group = 0;
    int curr_subgroup = 0;
    qreal scene_width;
    qreal cell_size = 80;

    DisplayMode mode;
    QDir image_path;
    QString image_suffix = ".png";
};

glaxnimate::emoji::EmojiDialog::EmojiDialog(QWidget *parent)
    : QDialog(parent), d(std::make_unique<Private>(this))
{
}

glaxnimate::emoji::EmojiDialog::~EmojiDialog()
{

}

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
}

QString glaxnimate::emoji::EmojiDialog::current_slug() const
{
    return d->current_slug;
}

void glaxnimate::emoji::EmojiDialog::load_emoji(glaxnimate::emoji::EmojiDialog::DisplayMode mode)
{
    d->mode = mode;
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

