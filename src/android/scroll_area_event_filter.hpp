#ifndef SCROLL_AREA_EVENT_FILTER_HPP
#define SCROLL_AREA_EVENT_FILTER_HPP

#include <QObject>
#include <QAbstractScrollArea>

#include <memory>

namespace glaxnimate::android {

class ScrollAreaEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit ScrollAreaEventFilter(QAbstractScrollArea *target = nullptr, Qt::Orientations direction = Qt::Vertical);
    ~ScrollAreaEventFilter();

    void set_target(QAbstractScrollArea* target);

signals:
    void clicked(QPoint p);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::android

#endif // SCROLL_AREA_EVENT_FILTER_HPP
