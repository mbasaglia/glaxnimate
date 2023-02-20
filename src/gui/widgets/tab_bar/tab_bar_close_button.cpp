/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tab_bar_close_button.hpp"

#include <QStyle>
#include <QTabBar>
#include <QPainter>
#include <QStyleOptionTab>

using namespace glaxnimate::gui;


static void initStyleOption(QTabBar* q, QStyleOptionTab *option, int tabIndex)
{
    const int totalTabs = q->count();

    if (!option || (tabIndex < 0 || tabIndex >= totalTabs))
        return;

    option->initFrom(q);
    option->rect = q->tabRect(tabIndex);
    option->row = 0;
    option->shape = q->shape();
    option->iconSize = q->iconSize();
    option->documentMode = q->documentMode();
    option->selectedPosition = QStyleOptionTab::NotAdjacent;
    option->position = QStyleOptionTab::Middle;
    QRect textRect = q->style()->subElementRect(QStyle::SE_TabBarTabText, option, q);
    option->text = q->fontMetrics().elidedText(option->text, q->elideMode(), textRect.width(), Qt::TextShowMnemonic);
}


void TabBarCloseButton::add_button(QTabBar* bar, int index)
{
    QStyleOptionTab opt;
    initStyleOption(bar, &opt, index);
    QTabBar::ButtonPosition close_side = QTabBar::ButtonPosition(bar->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, bar));
    QAbstractButton *button = new TabBarCloseButton(bar);
    connect(button, &QAbstractButton::clicked, bar, [bar, button, close_side]{
        for ( int i = 0; i < bar->count(); i++ )
        {
            if ( bar->tabButton(i, close_side) == button )
                bar->tabCloseRequested(i);
        }
    });
    bar->setTabButton(index, close_side, button);
}


TabBarCloseButton::TabBarCloseButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Delete Composition"));

    resize(sizeHint());
}

QSize TabBarCloseButton::sizeHint() const
{
    ensurePolished();
    int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, nullptr, this);
    int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, nullptr, this);
    return QSize(width, height);
}

#if QT_VERSION_MAJOR < 6
void TabBarCloseButton::enterEvent(QEvent * event)
#else
void TabBarCloseButton::enterEvent(QEnterEvent * event)
#endif
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void TabBarCloseButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void TabBarCloseButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    opt.state |= QStyle::State_AutoRaise;
    if (isEnabled() && underMouse() && !isChecked() && !isDown())
        opt.state |= QStyle::State_Raised;
    if (isChecked())
        opt.state |= QStyle::State_On;
    if (isDown())
        opt.state |= QStyle::State_Sunken;

    if (const QTabBar *tb = qobject_cast<const QTabBar *>(parent())) {
        int index = tb->currentIndex();
        QTabBar::ButtonPosition position = (QTabBar::ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, tb);
        if (tb->tabButton(index, position) == this)
            opt.state |= QStyle::State_Selected;
    }

    style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
}
