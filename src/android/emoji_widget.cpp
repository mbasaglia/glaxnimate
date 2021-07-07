#include "emoji_widget.hpp"

#include <QToolButton>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QScrollBar>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QScreen>

#include <QGraphicsSimpleTextItem>

#include "emoji_data.hpp"

class glaxnimate::android::EmojiWidget::Private
{
public:
    Private(EmojiWidget* parent) : parent(parent)
    {
        QVBoxLayout* lay = new QVBoxLayout(parent);
        parent->setLayout(lay);

        title = new QHBoxLayout;
        lay->addLayout(title);

        font.setPixelSize(font_size);
        int pad = 10;

        scene_width = columns * font_size + pad;


        section_font.setBold(true);

        table = new QGraphicsView(parent);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        lay->addWidget(table);
        table->setScene(&scene);


        scroller.set_target(table);
        table->viewport()->installEventFilter(&scroller);
        connect(&scroller, &ScrollAreaEventFilter::clicked, parent,
            [parent, this](const QPoint& pos){
                if ( auto item = scene.itemAt(table->mapToScene(pos), table->viewportTransform()) )
                {
                    for ( auto lab : section_headers )
                        if ( lab == item )
                            return;

                    emoji = static_cast<QGraphicsSimpleTextItem*>(item)->text();
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
                QPoint p = table->mapFromScene(group_label->pos());
                table->verticalScrollBar()->setValue(p.y() + table->verticalScrollBar()->value());
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

    void load_step()
    {
        const auto& grp = EmojiGroup::table[curr_group];


        if ( curr_subgroup == 0 )
        {
            section_headers[curr_group]->setVisible(true);
            section_headers[curr_group]->setPos(0, row*font_size);
            row++;
        }

        const auto& sub = grp.children[curr_subgroup];

        for ( const auto& emoji : sub.emoji )
        {
            auto label = scene.addSimpleText(emoji.unicode);
            auto rect = label->boundingRect();

            if ( rect.width() > font.pixelSize() * 1.25 )
            {
                delete label;
                continue;
            }

            label->setPos(QPointF(column*font_size, row*font_size) - rect.topLeft());
            label->setFont(font);

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

    QString emoji;
    ScrollAreaEventFilter scroller;
    EmojiWidget* parent;

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
    qreal font_size = 80;
};

glaxnimate::android::EmojiWidget::EmojiWidget(QWidget *parent)
    : BaseDialog(parent), d(std::make_unique<Private>(this))
{
    d->load_step();
    startTimer(20);
}

glaxnimate::android::EmojiWidget::~EmojiWidget()
{

}

QString glaxnimate::android::EmojiWidget::selected() const
{
    return d->emoji;
}

void glaxnimate::android::EmojiWidget::timerEvent(QTimerEvent *event)
{
    d->load_step();
    if ( d->curr_group >= int(EmojiGroup::table.size()) )
        killTimer(event->timerId());
}

void glaxnimate::android::EmojiWidget::showEvent(QShowEvent *e)
{
    BaseDialog::showEvent(e);
    d->on_rotate();
}
