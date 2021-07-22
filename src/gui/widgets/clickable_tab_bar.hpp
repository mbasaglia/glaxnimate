#pragma once

#include <QTabBar>

namespace glaxnimate::gui {

class ClickableTabBar : public QTabBar
{
    Q_OBJECT

public:
    using QTabBar::QTabBar;

protected:
    void mouseReleaseEvent(QMouseEvent * event) override;

signals:
    void context_menu_requested(int index);
};

} // namespace glaxnimate::gui
