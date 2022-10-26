#ifndef FRAMECONTROLSWIDGET_H
#define FRAMECONTROLSWIDGET_H

#include <QWidget>
#include <memory>
#include <chrono>


namespace glaxnimate::gui {

namespace Ui {
class FrameControlsWidget;
}

class FrameControlsWidget : public QWidget
{
    Q_OBJECT

public:
    FrameControlsWidget(QWidget* parent = nullptr);
    ~FrameControlsWidget();

    void set_range(int min, int max);
    void set_min(int min);
    void set_max(int max);
    void set_fps(qreal fps);
    void set_frame(int frame);

public slots:
    void play();
    void pause();
    void toggle_play();
    void set_record_enabled(bool enabled);
    void set_loop(bool loop);

    void go_first();
    void go_prev();
    void go_next();
    void go_last();

signals:
    void frame_selected(int frame);
    void record_toggled(bool enabled);
    void play_started();
    void play_stopped();
    void loop_changed(bool b);
    void min_changed(int min);
    void max_changed(int max);
    void fps_changed(qreal fps);

private slots:
    void play_toggled(bool play);
    void commit_time();


protected:
    void timerEvent(QTimerEvent* e) override;

protected:
    void changeEvent ( QEvent* e ) override;

private:
    qreal fps = 60;
    int timer = 0;
    std::chrono::high_resolution_clock::time_point playback_start;
    std::chrono::milliseconds playback_tick{17};
    std::unique_ptr<Ui::FrameControlsWidget> d;
};

} // namespace glaxnimate::gui

#endif // FRAMECONTROLSWIDGET_H
