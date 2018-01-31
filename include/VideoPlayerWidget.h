//
// Created by fred on 17/12/17.
//

#ifndef SFTPMEDIASTREAMER_PLAYBARWIDGET_H
#define SFTPMEDIASTREAMER_PLAYBARWIDGET_H


#include <gtkmm/box.h>
#include "VideoWidget.h"
#include "VideoControlWidget.h"

class VideoPlayerWidget : public Gtk::Box
{
public:
    VideoPlayerWidget(size_t parent_window_id, sf::InputStream *stream);
    ~VideoPlayerWidget() override;
    VideoWidget::state_update_signal_t signal_playback_state_changed();

private:
    void on_realize() override;
    void callback_play_state_changed(VideoWidget::SignalType type);


    VideoWidget video;
    VideoControlWidget video_controller;
    std::unique_ptr<Gtk::Window> controller_window;
    std::chrono::system_clock::time_point last_cursor_move_time;
    bool controller_hidden;
};


#endif //SFTPMEDIASTREAMER_PLAYBARWIDGET_H
