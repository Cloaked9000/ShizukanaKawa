//
// Created by fred on 17/12/17.
//

#ifndef SFTPMEDIASTREAMER_VIDEOPLAYERWIDGET_H
#define SFTPMEDIASTREAMER_VIDEOPLAYERWIDGET_H


#include <memory>
#include <gtkmm/widget.h>
#include "VideoPlayer.h"
#include <gdk/gdkx.h>

class VideoWidget : public Gtk::Widget, public VideoPlayer
{
public:
    enum SignalType
    {
        Playing,
        Paused,
        Stopped,
        FullscreenEntered,
        FullscreenExited,
        MouseMoved,
        Tick
    };
    typedef sigc::signal<void, SignalType> state_update_signal_t;

    /*!
     * Constructs a new video widget
     *
     * @param parent_window_id The window ID of the parent, for re-parenting the video player
     * to the parent. Or 0 to play in a dedicated window.
     *
     * @param stream The video stream to play
     */
    VideoWidget(size_t parent_window_id, sf::InputStream *stream);
    ~VideoWidget() override;

    state_update_signal_t signal_playback_state_changed();

    //Public overrides to allow for Gtk signals to be emitted

    /*!
     * Plays the video
     */
    void play() override;

    /*!
     * Pauses the video
     */
    void pause() override;

    /*!
     * Stops the video
     */
    void stop() override;

    /*!
     * Toggles fullscreen on and off
     */
    void toggle_fullscreen() override;
protected:

    /*!
     * Called internally to emit a mouse moved signal, when
     * the mouse is moved.
     *
     * @param x The X parameter of the mouse
     * @param y The Y parameter of the mouse
     */
    void mouse_moved(size_t x, size_t y) override;

    /*!
     * A GTK callback for processing key events
     *
     * @param key The key event
     * @return True if the event was consumed, false otherwise.
     */
    bool key_press_callback(GdkEventKey *key);

    //Overrides:
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const override;
    void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const  override;
    void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const override;
    void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const override;
    void on_size_allocate(Gtk::Allocation& allocation) override;
    void on_map() override;
    void on_unmap() override;
    void on_realize() override;
    void on_unrealize() override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

    //State
    Glib::RefPtr<Gdk::Window> gdk_window;
    sigc::connection video_updater;
    state_update_signal_t state_update_signal;
};


#endif //SFTPMEDIASTREAMER_VIDEOPLAYERWIDGET_H
