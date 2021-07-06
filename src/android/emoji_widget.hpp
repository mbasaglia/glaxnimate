#ifndef GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
#define GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP

#include <QDialog>
#include "scroll_area_event_filter.hpp"
#include "base_dialog.hpp"

class QAbstractScrollArea;

namespace glaxnimate::android {

class EmojiWidget : public BaseDialog
{
    Q_OBJECT

public:
    EmojiWidget(QWidget* parent = nullptr);
    ~EmojiWidget();

    QString selected() const;

protected:
    void timerEvent(QTimerEvent *event) override;
    void showEvent(QShowEvent* e) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
