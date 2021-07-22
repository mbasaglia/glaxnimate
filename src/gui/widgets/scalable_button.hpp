#pragma once

#include <QToolButton>
#include <QApplication>
#include <QStylePainter>
#include <QStyleOptionToolButton>

namespace glaxnimate::gui {


class ScalableButton : public QToolButton
{
public:
    using QToolButton::QToolButton;

    QSize sizeHint() const override
    {
        return QSize(32, width()).expandedTo(QApplication::globalStrut());
    }

protected:
    void resizeEvent ( QResizeEvent * ev ) override
    {
        updateGeometry();
        QToolButton::resizeEvent(ev);
    }

    void paintEvent ( QPaintEvent * ) override
    {
        QStylePainter p(this);
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        int pad = 16;
        int icon_size = std::min(width() - pad, height() - pad);
        opt.iconSize = QSize(icon_size, icon_size);
        p.drawComplexControl(QStyle::CC_ToolButton, opt);
    }
};

} // namespace glaxnimate::gui
