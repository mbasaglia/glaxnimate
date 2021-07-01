#ifndef GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
#define GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP

#include <QDialog>

class QAbstractScrollArea;

namespace glaxnimate::android {

class EmojiWidget : public QDialog
{
    Q_OBJECT

public:
    EmojiWidget(QWidget* parent = nullptr);

    QString selected() const;

protected:
    bool eventFilter(QObject * object, QEvent * event) override;

private:
    QString emoji;
    int scroll_start;
    QAbstractScrollArea* table_area;
};

} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_EMOJIWIDGET_HPP
