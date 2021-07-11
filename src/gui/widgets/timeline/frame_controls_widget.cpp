#include "frame_controls_widget.hpp"
#include "ui_frame_controls_widget.h"

#include <cmath>

#include "glaxnimate_app.hpp"

FrameControlsWidget::FrameControlsWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Ui::FrameControlsWidget>())
{
    d->setupUi(this);

#ifdef Q_OS_ANDROID
    d->layout->setMargin(0);
    d->layout->setSpacing(0);
#endif

    d->button_record->setIcon(QIcon(GlaxnimateApp::instance()->data_file("images/icons/keyframe-record.svg")));

    d->button_next_kf->setVisible(false);
    d->button_prev_kf->setVisible(false);
    connect(d->button_record, &QAbstractButton::clicked, this, &FrameControlsWidget::record_toggled);
    connect(d->button_loop, &QAbstractButton::clicked, this, &FrameControlsWidget::loop_changed);
}

FrameControlsWidget::~FrameControlsWidget() = default;

void FrameControlsWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        d->retranslateUi(this);
    }
}

void FrameControlsWidget::play_toggled(bool checked)
{
    if ( checked )
        play();
    else
        pause();
}

void FrameControlsWidget::set_min(int min)
{
    d->spin_frame->setMinimum(min);
}

void FrameControlsWidget::set_max(int max)
{
    d->spin_frame->setMaximum(max);
}

void FrameControlsWidget::set_range(int min, int max)
{
    d->spin_frame->setRange(min, max);
    d->spin_frame->setValue(min);
}

void FrameControlsWidget::set_fps(qreal fps)
{
    this->fps = fps;
}

void FrameControlsWidget::timerEvent(QTimerEvent*)
{
    if ( d->spin_frame->maximum() <= d->spin_frame->minimum() || fps <= 0 )
        return;

    auto range_frames = d->spin_frame->maximum() - d->spin_frame->minimum();

    std::chrono::duration<double> offset = std::chrono::high_resolution_clock::now() - playback_start;
    auto seconds_off = offset.count();
    qreal frame_off = seconds_off * fps;

    if ( frame_off >= range_frames && !d->button_loop->isChecked() )
    {
        pause();
        d->spin_frame->setValue(d->spin_frame->minimum());
        return;
    }

    d->spin_frame->setValue(d->spin_frame->minimum() + qRound(std::fmod(frame_off, range_frames)));
}

void FrameControlsWidget::play()
{
    if ( !timer )
    {
        timer = startTimer(playback_tick, Qt::PreciseTimer);
        playback_start = std::chrono::high_resolution_clock::now();
        d->button_play->setChecked(true);
        d->button_play->setIcon(QIcon::fromTheme("media-playback-pause"));
        emit play_started();
    }
}

void FrameControlsWidget::pause()
{
    if ( timer )
    {
        killTimer(timer);
        timer = 0;
        d->button_play->setChecked(false);
        d->button_play->setIcon(QIcon::fromTheme("media-playback-start"));
        emit play_stopped();
    }
}

void FrameControlsWidget::commit_time()
{
    emit frame_selected(d->spin_frame->value());
}

void FrameControlsWidget::go_first()
{
    d->spin_frame->setValue(d->spin_frame->minimum());
}

void FrameControlsWidget::go_last()
{
    d->spin_frame->setValue(d->spin_frame->maximum());
}

void FrameControlsWidget::go_next()
{
    int i = d->spin_frame->value();
    if ( i >= d->spin_frame->maximum() && d->button_loop->isChecked() )
        d->spin_frame->setValue(d->spin_frame->minimum());
    else
        d->spin_frame->setValue(i+1);
}

void FrameControlsWidget::go_prev()
{
    int i = d->spin_frame->value();
    if ( i <= d->spin_frame->minimum() && d->button_loop->isChecked() )
        d->spin_frame->setValue(d->spin_frame->maximum());
    else
        d->spin_frame->setValue(i-1);
}

void FrameControlsWidget::set_frame(int frame)
{
    d->spin_frame->setValue(frame);
}

void FrameControlsWidget::set_record_enabled(bool enabled)
{
    d->button_record->setChecked(enabled);
}

void FrameControlsWidget::toggle_play()
{
    if ( timer )
        pause();
    else
        play();
}

void FrameControlsWidget::set_loop(bool loop)
{
    d->button_loop->setChecked(loop);
}
