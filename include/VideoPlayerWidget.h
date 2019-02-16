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
    VideoPlayerWidget(size_t parent_window_id, std::unique_ptr<sf::InputStream> stream);
    ~VideoPlayerWidget() override;
    VideoWidget::state_update_signal_t signal_playback_state_changed();

    /*!
     * Gets the ID of the current audio track
     *
     * @return ID of the current audio track
     */
    inline size_t get_audio_track()
    {
        return video->get_audio_track();
    }

    /*!
     * Gets the ID of the current sub track
     *
     * @return ID of the current sub track
     */
    inline size_t get_subtitle_track()
    {
        return video->get_subtitle_track();
    }

    /*!
     * Gets the current playback offset in milliseconds
     *
     * @return The current playback offset in milliseconds
     */
    inline size_t get_playback_offset()
    {
        return video->get_playback_offset();
    }

    /*!
     * Sets the ID of the current audio track
     *
     * @param track_id ID of the audio track to use
     */
    inline void set_audio_track(size_t track_id)
    {
        video->set_audio_track(track_id);
    }

    /*!
     * Sets the ID of the current sub track
     *
     * @param sub_id The ID of the subtitle track to switch to
     */
    inline void set_subtitle_track(size_t sub_id)
    {
        video->set_subtitle_track(sub_id);
    }

    /*!
     * Sets the current playback offset in milliseconds
     *
     * @param offset The playback offset to seek to in milliseconds
     */
    inline void set_playback_offset(size_t offset)
    {
        video->set_playback_offset(offset);
    }

private:
    void on_realize() override;
    void callback_play_state_changed(VideoWidget::SignalType type);


    std::shared_ptr<VideoWidget> video;
    std::unique_ptr<VideoControlWidget> video_controller;
    std::unique_ptr<Gtk::Window> controller_window;
    std::chrono::system_clock::time_point last_cursor_move_time;
    bool controller_hidden;

    sigc::connection state_changed;
};


#endif //SFTPMEDIASTREAMER_PLAYBARWIDGET_H
