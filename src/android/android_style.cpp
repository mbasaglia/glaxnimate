#include "android_style.hpp"

#include <QStyleOptionSlider>
#include <QPainter>
#include <QTextLayout>

int glaxnimate::android::AndroidStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch ( metric )
    {
        case PM_SmallIconSize:
        case PM_LargeIconSize:
        case PM_IconViewIconSize:
        case PM_ListViewIconSize:
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
            return 80;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QRect glaxnimate::android::AndroidStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const
{
    auto rect = QProxyStyle::subControlRect(cc, opt, sc, widget);

    switch ( sc )
    {
        case SC_ScrollBarAddLine:
        {
            auto option = static_cast<const QStyleOptionSlider*>(opt);
            if ( option->orientation == Qt::Horizontal )
            {
                return QRect(
                option->rect.topRight() - QPoint(option->rect.height(), 0),
                option->rect.bottomRight()
                );
            }
            break;
        }
        case SC_ScrollBarSubLine:
        {
            auto option = static_cast<const QStyleOptionSlider*>(opt);
            if ( option->orientation == Qt::Horizontal )
            {
                return QRect(
                option->rect.topLeft(),
                option->rect.bottomLeft() + QPoint(option->rect.height(), 0)
                );
            }
            break;
        }
        case SC_ScrollBarGroove:
        {
            auto option = static_cast<const QStyleOptionSlider*>(opt);
            if ( option->orientation == Qt::Horizontal )
            {
                return QRect(
                option->rect.topLeft() + QPoint(option->rect.height(), 0),
                option->rect.bottomRight() - QPoint(option->rect.height(), 0)
                );
            }
            break;
        }
        case SC_ScrollBarSlider:
        {
            auto option = static_cast<const QStyleOptionSlider*>(opt);
            if ( option->minimum < option->maximum )
            {
                qreal delta = option->maximum - option->minimum;
                qreal factor = (option->sliderValue - option->minimum) / delta;
                if ( option->orientation == Qt::Horizontal )
                {
                    qreal avail = option->rect.width() - option->rect.height() * 3;
                    int x = qRound(factor * avail + option->rect.left() + option->rect.height());

                    return QRect(
                    QPoint(x, option->rect.top()),
                    QPoint(x+option->rect.height(), option->rect.bottom())
                    );

                }
            }
            break;
        }
        default:
            break;
    }

    return rect;
}

QSize glaxnimate::android::AndroidStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    QSize sz = QProxyStyle::sizeFromContents(type, option, size, widget);
    if ( type == CT_ItemViewItem && size.isValid() )
    {
        if ( const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option) )
            return sz.expandedTo({80, 80});
    }

    return sz;
}

QRect glaxnimate::android::AndroidStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    auto rect = QProxyStyle::subElementRect(element, option, widget);
    if ( element == SE_ItemViewItemDecoration )
        if (const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(option))
            return QRect(option->rect.topLeft(), QSize(80, 80));
    return rect;
}

void glaxnimate::android::AndroidStyle::drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *widget) const
{
    if ( element != CE_ItemViewItem )
        return QProxyStyle::drawControl(element, opt, p, widget);

    const QStyleOptionViewItem *vopt = qstyleoption_cast<const QStyleOptionViewItem *>(opt);
    if ( !vopt )
        return;

    QStyleOptionViewItem copy = *vopt;
    copy.decorationSize = QSize();
    copy.icon = {};
    QProxyStyle::drawControl(element, &copy, p, widget);

    if ( !vopt->icon.isNull() )
    {
        p->save();
        p->setClipRect(opt->rect);
        QRect iconRect = subElementRect(SE_ItemViewItemDecoration, vopt, widget);
        auto pix = vopt->icon.pixmap(iconRect.size());
        p->drawPixmap(iconRect, pix);
        p->restore();
    }
}
