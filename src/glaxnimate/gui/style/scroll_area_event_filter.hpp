#ifndef SCROLL_AREA_EVENT_FILTER_HPP
#define SCROLL_AREA_EVENT_FILTER_HPP

#include <QObject>
#include <QAbstractScrollArea>

#include <memory>

class QScroller;

namespace glaxnimate::gui {

/**
 * \brief Adds touch scroll support to scroll areas
 */
class ScrollAreaEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit ScrollAreaEventFilter(QAbstractScrollArea *target = nullptr, Qt::Orientations direction = Qt::Vertical);
    ~ScrollAreaEventFilter();

    void set_target(QAbstractScrollArea* target);

    void scroll_to(const QPointF& p);

    static QScroller* setup_scroller(QAbstractScrollArea* target);

signals:
    void clicked(QPoint p);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::gui

#endif // SCROLL_AREA_EVENT_FILTER_HPP
