#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>
#include <memory>

class GlaxnimateWindowPrivate;

class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GlaxnimateWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    ~GlaxnimateWindow();

protected:
    void changeEvent(QEvent *e) override;

    bool eventFilter(QObject *object, QEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // GLAXNIMATEWINDOW_H
