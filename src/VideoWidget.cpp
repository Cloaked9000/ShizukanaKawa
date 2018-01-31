//
// Created by fred on 17/12/17.
//

#include <iostream>
#include <giomm.h>
#include <thread>
#include "VideoWidget.h"


VideoWidget::VideoWidget(size_t parent_window_id, sf::InputStream *stream)
: Glib::ObjectBase("videoplayer"),
  Gtk::Widget(),
  VideoPlayer(parent_window_id)
{
    set_has_window(true);
    set_hexpand(true);
    set_vexpand(true);

    //Open video
    open_from_stream(*stream, {});

    //Setup update to be called periodically
    video_updater = Glib::signal_timeout().connect([&]() -> bool {update(); state_update_signal.emit(SignalType::Tick); return true;}, 50);

    //Setup callbacks
    signal_key_press_event().connect(sigc::mem_fun(this, &VideoWidget::key_press_callback));
}

VideoWidget::~VideoWidget()
{
    video_updater.disconnect();
}

Gtk::SizeRequestMode VideoWidget::get_request_mode_vfunc() const
{
    return Widget::get_request_mode_vfunc();
}

void VideoWidget::get_preferred_width_vfunc(int &minimum_width, int &natural_width) const
{
    minimum_width = 0;
    natural_width = get_resolution().x;
    natural_width = 10000;
}

void VideoWidget::get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const
{
    minimum_height = 0;
    natural_height = static_cast<int>(width * 0.5625f);
    natural_height = 10000;
}

void VideoWidget::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const
{
    minimum_height = 0;
    natural_height = get_resolution().y;
    natural_height = 10000;
}


void VideoWidget::get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const
{
    Widget::get_preferred_width_for_height_vfunc(height, minimum_width, natural_width);
}

void VideoWidget::on_size_allocate(Gtk::Allocation &allocation)
{
    set_allocation(allocation);
    int x, y;
    gtk_widget_translate_coordinates(gobj(), gtk_widget_get_toplevel(gobj()), 0, 0, &x, &y);
    auto size = sf::Vector2u(allocation.get_width(), allocation.get_height());
    auto position = sf::Vector2i(x, y);

    set_size(size);
    set_position(position);

    if(gdk_window)
    {
        gdk_window->move_resize(allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
    }
}

void VideoWidget::on_map()
{
    Widget::on_map();
}

void VideoWidget::on_unmap()
{
    Widget::on_unmap();
}

void VideoWidget::on_realize()
{
    set_realized();

    set_can_focus(true);
    grab_focus();

    if(!gdk_window)
    {
        //Create a window
        GdkWindowAttr attributes = {};
        memset(&attributes, 0, sizeof(attributes));
        Gtk::Allocation allocation = get_allocation();

        //Set initial position and size of the Gdk::Window:
        attributes.x = allocation.get_x();
        attributes.y = allocation.get_y();
        attributes.width = allocation.get_width();
        attributes.height = allocation.get_height();

        attributes.event_mask = get_events () | Gdk::EXPOSURE_MASK;
        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.wclass = GDK_INPUT_OUTPUT;

        gdk_window = Gdk::Window::create(get_parent_window(), &attributes, GDK_WA_X | GDK_WA_Y);
        set_window(gdk_window);

        //Make it receive events
        gdk_window->set_user_data(gobj());
    }
}

void VideoWidget::on_unrealize()
{
    gdk_window.reset();
    Widget::on_unrealize();
}

bool VideoWidget::on_draw(const Cairo::RefPtr<Cairo::Context> &)
{
    update();
    return true;
}

void VideoWidget::play()
{
    VideoPlayer::play();
    state_update_signal.emit(SignalType::Playing);
}

void VideoWidget::pause()
{
    VideoPlayer::pause();
    state_update_signal.emit(SignalType::Paused);
}

void VideoWidget::stop()
{
    VideoPlayer::stop();
    state_update_signal.emit(SignalType::Stopped);
}

void VideoWidget::mouse_moved(size_t x, size_t y)
{
    VideoPlayer::mouse_moved(x, y);
    state_update_signal.emit(SignalType::MouseMoved);
}

void VideoWidget::toggle_fullscreen()
{
    VideoPlayer::toggle_fullscreen();
    if(is_fullscreen())
        state_update_signal.emit(SignalType::FullscreenEntered);
    else
        state_update_signal.emit(SignalType::FullscreenExited);
}

bool VideoWidget::key_press_callback(GdkEventKey *key)
{
    switch(key->keyval)
    {
        case GDK_KEY_f:
        case GDK_KEY_F:
            toggle_fullscreen();
            return true;

        case GDK_KEY_b:
        case GDK_KEY_B:
            rotate_audio_track();
            return true;

        case GDK_KEY_v:
        case GDK_KEY_V:
            rotate_subtitle_track();
            return true;

        case GDK_KEY_s:
        case GDK_KEY_S:
            save_screenshot({});
            return true;

        case GDK_KEY_space:
            toggle_pause();
            return true;
        default:
            break;
    }
    return false;
}

VideoWidget::state_update_signal_t VideoWidget::signal_playback_state_changed()
{
    return state_update_signal;
}
