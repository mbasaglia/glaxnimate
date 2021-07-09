#ifndef ANDROID_STYLE_HPP
#define ANDROID_STYLE_HPP

#include <QProxyStyle>

namespace glaxnimate::android {

class AndroidStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr, const QWidget *widget = nullptr) const override;

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const override;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override;

    QRect subElementRect(QStyle::SubElement element, const QStyleOption *option, const QWidget *widget) const override;

    void drawControl(ControlElement element, const QStyleOption *opt,
                     QPainter *p, const QWidget *widget) const override;

};

} // namespace glaxnimate::android
#endif // ANDROID_STYLE_HPP
