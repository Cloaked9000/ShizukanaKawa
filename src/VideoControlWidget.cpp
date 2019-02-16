//
// Created by fred on 31/12/17.
//

#include <giomm.h>
#include <VideoPlayer.h>
#include <iostream>
#include <thread>
#include <Types.h>
#include "VideoControlWidget.h"

VideoControlWidget::VideoControlWidget(std::shared_ptr<VideoWidget> video_)
: video(std::move(video_))
{
    pause_icon = Gdk::Pixbuf::create_from_file("resources/pause_icon.png");
    play_icon = Gdk::Pixbuf::create_from_file("resources/play_icon.png");
    stop_icon = Gdk::Pixbuf::create_from_file("resources/stop_icon.png");

    pause_button.set(pause_icon);
    pause_button_box.add(pause_button);
    pause_button_signal = pause_button_box.signal_button_press_event().connect(sigc::mem_fun(*this, &VideoControlWidget::pause_button_click_callback));

    stop_button.set(stop_icon);
    stop_button_box.add(stop_button);
    stop_button_signal = stop_button_box.signal_button_press_event().connect(sigc::mem_fun(*this, &VideoControlWidget::stop_button_click_callback));

    seek_bar_update_clock = Glib::signal_timeout().connect(sigc::mem_fun(this, &VideoControlWidget::update_seek_bar), 100);
    seek_bar_box.add(seek_bar);
    seek_bar_signal = seek_bar_box.signal_button_press_event().connect(sigc::mem_fun(this, &VideoControlWidget::seek_bar_click_callback));

    button_control_box.set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    button_control_box.add(video_offset_label);
    button_control_box.add(pause_button_box);
    button_control_box.add(stop_button_box);
    button_control_box.add(video_duration_label);
    button_control_box.set_halign(Gtk::Align::ALIGN_CENTER);

    add(seek_bar_box);
    add(button_control_box);
    set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
    set_hexpand(true);
}

VideoControlWidget::~VideoControlWidget()
{
    pause_button_signal.disconnect();
    stop_button_signal.disconnect();
    seek_bar_signal.disconnect();
}

bool VideoControlWidget::update_seek_bar()
{
    if(!video)
        return false;

    //Update UI buttons as the video can change them independently of us
    if(video->get_playback_state() == VideoPlayer::Paused)
        pause_button.set(play_icon);
    else
        pause_button.set(pause_icon);

    //Update progress bar and time offset label
    if(video->get_playback_state() != VideoPlayer::Stopped)
    {
        ssize_t playback_offset = video->get_playback_offset();
        ssize_t video_duration = video->get_duration();
        if(video_duration <= 0 || playback_offset < 0)
            return true;

        seek_bar.set_fraction(video->get_playback_offset() / static_cast<double>(video_duration));
        if(video_duration_label.get_text().empty())
            video_duration_label.set_text(seconds_to_text(static_cast<size_t>(video_duration / 1000)));
        video_offset_label.set_text(seconds_to_text(static_cast<size_t>(playback_offset / 1000)));
    }

    return true;
}

bool VideoControlWidget::pause_button_click_callback(GdkEventButton *)
{
    if(!video)
        return false;

    if(video->get_playback_state() == VideoPlayer::Paused)
        video->play();
    else
        video->pause();
    update_seek_bar();

    return true;
}

bool VideoControlWidget::stop_button_click_callback(GdkEventButton *)
{
    if(!video || video->is_fullscreen()) //Disabled in fullscreen mode at the moment as it causes a crash
        return true;


    video->stop();

    return true;
}

bool VideoControlWidget::seek_bar_click_callback(GdkEventButton *event)
{
    if(!video)
        return false;

    //todo: Not very portable. Should not assume that the seek bar takes up the whole width of the window.
    double seek_percentage = event->x / static_cast<double>(get_window()->get_width());
    auto seek_offset_ms = static_cast<uint64_t>(seek_percentage * video->get_duration());
    video->set_playback_offset(seek_offset_ms);
    update_seek_bar();
    return true;
}

std::string VideoControlWidget::seconds_to_text(size_t input)
{
    std::string hour;
    std::string min;
    std::string sec;
    std::string ret;

    size_t hours = input / Duration::Hour;
    input -= hours * Duration::Hour;
    size_t minutes = input / Duration::Minute;
    input -= minutes * Duration::Minute;
    size_t seconds = input / Duration::Second;

    hour = hours > 9 ?  std::to_string(hours) : "0" + std::to_string(hours);
    min = minutes > 9 ?  std::to_string(minutes) : "0" + std::to_string(minutes);
    sec = seconds > 9 ?  std::to_string(seconds) : "0" + std::to_string(seconds);

    if(hours > 0)
        ret += hour + ":";
    ret += min + ":" + sec;
    return ret;
}
