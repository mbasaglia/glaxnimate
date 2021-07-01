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

#include <QDebug>

glaxnimate::android::EmojiWidget::EmojiWidget(QWidget *parent)
    : QDialog(parent)
{
    setWindowState(windowState() | Qt::WindowFullScreen);

    QVBoxLayout* lay = new QVBoxLayout(this);
    setLayout(lay);

    QHBoxLayout* title = new QHBoxLayout;
    lay->addLayout(title);

    int size = 64;
    QFont font;
    font.setPixelSize(size);
    int pad = 10;
    QSize size_hint(size, size+pad);

    QFont section_font;
    section_font.setBold(true);

    int row = 0;
    int column = 0;
    int columns = 8;

    auto table = new QScrollArea(this);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lay->addWidget(table);
    table_area = table;
    table->viewport()->installEventFilter(this);
    table->setWidgetResizable(true);
    auto table_inner = new QWidget();
    auto table_layout = new QGridLayout(table_inner);
    table_inner->setLayout(table_layout);

    table->setWidget(table_inner);

    for ( const auto& grp : EmojiGroup::table )
    {
        auto first = grp.children[0].emoji[0];
        QLabel* group_label = new QLabel(table_inner);
        group_label->setText(grp.name);
        group_label->setFont(section_font);
        table_layout->addWidget(group_label, row, 0, 1, columns);
        row++;

        QToolButton* btn = new QToolButton(this);
        btn->setText(first.unicode);
        connect(btn, &QAbstractButton::clicked, this, [table, group_label]{
            QPoint p = group_label->mapTo(table, QPoint(0,0));
            table->verticalScrollBar()->setValue(p.y() + table->verticalScrollBar()->value());
        });
        title->addWidget(btn);

        for ( const auto& sub : grp.children )
        {
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
        }
        row++;
        column = 0;
    }
}

QString glaxnimate::android::EmojiWidget::selected() const
{
    return emoji;
}

bool glaxnimate::android::EmojiWidget::eventFilter(QObject *object, QEvent *event)
{
    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
        {
            scroll_start = -1;
            return true;
        }
        case QEvent::MouseMove:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            if ( scroll_start != -1 )
            {
                table_area->verticalScrollBar()->setValue(
                        table_area->verticalScrollBar()->value() - mouse_event->pos().y() + scroll_start
                );
            }
            scroll_start = mouse_event->pos().y();
            return true;
        }
        case QEvent::MouseButtonRelease:
        {
            auto mouse_event = static_cast<QMouseEvent*>(event);
            QLabel* label = qobject_cast<QLabel*>(table_area->childAt(mouse_event->pos()));
            if ( label )
            {
                emoji = label->text();
                accept();
            }
            return true;
        }
        default:
            break;
    }

    return QDialog::eventFilter(object, event);
}
