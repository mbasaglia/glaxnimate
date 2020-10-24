#include "frame_controls_widget.hpp"
#include "ui_frame_controls_widget.h"

FrameControlsWidget::FrameControlsWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Ui::FrameControlsWidget>())
{
    d->setupUi(this);
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
    int i = d->spin_frame->value();
    if ( i >= d->spin_frame->maximum() )
    {
        d->spin_frame->setValue(d->spin_frame->minimum());

        if ( !d->button_loop->isChecked() )
            pause();
    }
    else
    {
        d->spin_frame->setValue(i+1);
    }
}

void FrameControlsWidget::play()
{
    if ( !timer )
    {
        timer = startTimer(1000.0 / fps, Qt::PreciseTimer);

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
    commit_time();
}

void FrameControlsWidget::go_last()
{
    d->spin_frame->setValue(d->spin_frame->maximum());
    commit_time();
}

void FrameControlsWidget::go_next()
{
    int i = d->spin_frame->value();
    if ( i >= d->spin_frame->maximum() && d->button_loop->isChecked() )
        d->spin_frame->setValue(d->spin_frame->minimum());
    else
        d->spin_frame->setValue(i+1);
    commit_time();
}

void FrameControlsWidget::go_prev()
{
    int i = d->spin_frame->value();
    if ( i <= d->spin_frame->minimum() && d->button_loop->isChecked() )
        d->spin_frame->setValue(d->spin_frame->maximum());
    else
        d->spin_frame->setValue(i-1);
    commit_time();
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
