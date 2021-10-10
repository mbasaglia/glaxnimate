#pragma once
#include <memory>
#include <QTreeView>

namespace glaxnimate::gui {

class TimelineTreeview : public QTreeView
{
public:
    explicit TimelineTreeview(QWidget* parent = nullptr);
    ~TimelineTreeview();

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void paintEvent(QPaintEvent * event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
