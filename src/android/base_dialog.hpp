#ifndef GLAXNIMATE_ANDROID_BASEDIALOG_HPP
#define GLAXNIMATE_ANDROID_BASEDIALOG_HPP

#include <QDialog>

namespace glaxnimate::android {

class BaseDialog : public QDialog
{
public:
    BaseDialog(QWidget* parent = nullptr);

    int exec() override;

protected:
    void paintEvent(QPaintEvent* ev) override;
    void keyReleaseEvent(QKeyEvent *) override;
    bool eventFilter(QObject * object, QEvent * event) override;
};


class DialogFixerFilter : public QObject
{
public:
    explicit DialogFixerFilter(QDialog *target = nullptr);

    void set_target(QDialog* target);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QDialog* target = nullptr;
};


} // namespace glaxnimate::android

#endif // GLAXNIMATE_ANDROID_BASEDIALOG_HPP
