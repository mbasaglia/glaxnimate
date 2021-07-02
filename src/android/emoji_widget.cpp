#include "emoji_widget.hpp"

#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QScrollBar>

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

        int size = 64;
        font.setPixelSize(size);
        int pad = 10;
        size_hint = QSize(size, size+pad);


        section_font.setBold(true);

        table = new QScrollArea(parent);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        lay->addWidget(table);
        table->viewport()->installEventFilter(parent);
        table->setWidgetResizable(true);
        table_inner = new QWidget();
        table_layout = new QGridLayout(table_inner);
        table_inner->setLayout(table_layout);

        table->setWidget(table_inner);


        scroller.set_target(table);
        connect(&scroller, &ScrollAreaEventFilter::clicked, parent,
            [parent, this](const QPoint& pos){
                if ( QLabel* label = qobject_cast<QLabel*>(table->childAt(pos)) )
                {
                    emoji = label->text();
                    parent->accept();
                }
        });

        for ( const auto& grp : EmojiGroup::table )
        {
            auto first = grp.children[0].emoji[0];
            QLabel* group_label = new QLabel();
            group_label->setText(grp.name);
            group_label->setFont(section_font);
            table_layout->addWidget(group_label, row, 0, 1, columns);
            row++;
            section_headers.push_back(group_label);

            QToolButton* btn = new QToolButton(parent);
            btn->setText(first.unicode);
            connect(btn, &QAbstractButton::clicked, parent, [this, group_label]{
                QPoint p = group_label->mapTo(table, QPoint(0,0));
                table->verticalScrollBar()->setValue(p.y() + table->verticalScrollBar()->value());
            });
            title->addWidget(btn);
        }

        row = 1;

        load_step();
    }

    void load_step()
    {
        const auto& grp = EmojiGroup::table[curr_group];
        const auto& sub = grp.children[curr_subgroup];

        for ( const auto& emoji : sub.emoji )
        {
            auto label = new QLabel(table_inner);
            label->setText(emoji.unicode);
            label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            label->setMinimumSize(size_hint);
            label->setFont(font);
            label->setAlignment(Qt::AlignCenter);
            table_layout->addWidget(label, row, column, Qt::AlignCenter);

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

            if ( curr_group < int(EmojiGroup::table.size()) )
                table_layout->addWidget(section_headers[curr_group], row, 0, 1, columns);

            row++;
            column = 0;
        }
    }

    QString emoji;
    ScrollAreaEventFilter scroller;
    EmojiWidget* parent;

    QFont font;
    QSize size_hint;
    QFont section_font;

    QHBoxLayout* title;
    QGridLayout* table_layout;
    QWidget* table_inner;
    QScrollArea* table;
    std::vector<QLabel*> section_headers;

    int row = 0;
    int column = 0;
    int columns = 8;
    int curr_group = 0;
    int curr_subgroup = 0;
};

glaxnimate::android::EmojiWidget::EmojiWidget(QWidget *parent)
    : BaseDialog(parent), d(std::make_unique<Private>(this))
{
    startTimer(100);
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
