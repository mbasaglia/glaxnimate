/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "better_toolbox_widget.hpp"

#include <QVBoxLayout>
#include <QToolButton>
#include <QButtonGroup>
#include <QStyle>
#include <QFrame>
#include <QEvent>

class glaxnimate::android::BetterToolboxWidget::Private
{
public:
    struct Item
    {
        QToolButton* title = nullptr;
        QWidget* widget = nullptr;
        QWidget* line = nullptr;
    };

    QWidget* make_line()
    {
        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        return line;
    }

    std::vector<Item> items;
    QVBoxLayout* layout;
    QButtonGroup group;
    int max_width = -1;
};

glaxnimate::android::BetterToolboxWidget::BetterToolboxWidget(QWidget *parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->layout = new QVBoxLayout(this);
    d->layout->setMargin(0);
    d->layout->setSpacing(0);
    setLayout(d->layout);
    d->group.setExclusive(true);
    d->layout->addWidget(d->make_line());
}

glaxnimate::android::BetterToolboxWidget::~BetterToolboxWidget()
{

}

QWidget *glaxnimate::android::BetterToolboxWidget::addItem(const QIcon &icon, const QString &text)
{
    auto widget = new QWidget(this);
    addItem(widget, icon, text);
    return widget;
}

void glaxnimate::android::BetterToolboxWidget::addItem(QWidget *widget, const QIcon &icon, const QString &text)
{
    int extent = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, nullptr);
    QSize sz(extent, extent);

    auto icon_label = new QToolButton(this);
    icon_label->setIcon(icon);
    icon_label->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    icon_label->setIconSize(sz);
    icon_label->setText(text);
    icon_label->setCheckable(true);
    icon_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    d->group.addButton(icon_label);
    icon_label->setChecked(d->items.empty());
    d->layout->addWidget(icon_label);
    d->layout->addWidget(d->make_line());

    d->layout->addWidget(widget);
//    d->max_width = qMax(widget->sizeHint().width(), d->max_width);
    widget->setVisible(icon_label->isChecked());
//    auto sp = widget->sizePolicy();
//    sp.setHorizontalPolicy(QSizePolicy::Expanding);
//    widget->setSizePolicy(sp);
    connect(icon_label, &QToolButton::toggled, widget, &QWidget::setVisible);

    QWidget* line = d->make_line();
    d->layout->addWidget(line);
    line->setVisible(icon_label->isChecked());
    connect(icon_label, &QToolButton::toggled, line, &QWidget::setVisible);

    d->items.push_back({icon_label, widget, line});
}

void glaxnimate::android::BetterToolboxWidget::addItem(QWidget *widget, const QString &text)
{
    addItem(widget, {}, text);
}

void glaxnimate::android::BetterToolboxWidget::setItemText(int index, const QString &text)
{
    d->items[index].title->setText(text);
}

int glaxnimate::android::BetterToolboxWidget::count() const
{
    return d->items.size();
}

QSize glaxnimate::android::BetterToolboxWidget::sizeHint() const
{
    QSize hint = QWidget::sizeHint();
    if ( d->max_width == -1 )
    {
        d->max_width = hint.width();
        for ( const auto & item : d->items )
            d->max_width = qMax(item.widget->sizeHint().width(), d->max_width);
    }

    return {d->max_width, hint.height()};
}

bool glaxnimate::android::BetterToolboxWidget::event(QEvent *event)
{
    if ( event->type() == QEvent::LayoutRequest )
        d->max_width = -1;

    return QWidget::event(event);
}
