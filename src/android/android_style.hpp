#ifndef ANDROID_STYLE_HPP
#define ANDROID_STYLE_HPP

#include <QProxyStyle>
#include <QStyleOptionSlider>

namespace glaxnimate::android {

class AndroidStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override
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
};

} // namespace glaxnimate::android
#endif // ANDROID_STYLE_HPP
