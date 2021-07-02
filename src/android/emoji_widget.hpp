#ifndef GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
#define GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP

#include <QDialog>
#include "scroll_area_event_filter.hpp"

class QAbstractScrollArea;

namespace glaxnimate::android {

class EmojiWidget : public QDialog
{
    Q_OBJECT

public:
    EmojiWidget(QWidget* parent = nullptr);

    QString selected() const;

private:
    QString emoji;
    ScrollAreaEventFilter scroller;
};

} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
