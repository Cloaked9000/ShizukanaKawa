//
// Created by fred on 17/12/17.
//

#include <iostream>
#include <gtkmm/window.h>
#include <VideoPlayerWidget.h>
#include <thread>

VideoPlayerWidget::VideoPlayerWidget(size_t parent_window_id, sf::InputStream *stream)
: video(parent_window_id, stream),
  video_controller(&video),
  last_cursor_move_time(std::chrono::system_clock::now()),
  controller_hidden(false)
{
    //Add player and controller
    set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
    add(video);
    add(video_controller);

    //Setup callbacks to monitor playback
    video.signal_playback_state_changed().connect(sigc::mem_fun(this, &VideoPlayerWidget::callback_play_state_changed));
}

VideoPlayerWidget::~VideoPlayerWidget()
{

}


void VideoPlayerWidget::on_realize()
{
    Gtk::Box::on_realize();
    set_can_focus(true);
    grab_focus();
}

VideoWidget::state_update_signal_t VideoPlayerWidget::signal_playback_state_changed()
{
    return video.signal_playback_state_changed();
}

void VideoPlayerWidget::callback_play_state_changed(VideoWidget::SignalType type)
{
    switch(type)
    {
        case VideoWidget::FullscreenEntered:
        {
            last_cursor_move_time = std::chrono::system_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); //todo: This should not be needed. It takes a little bit for get_render_window_size() to update

            //Move the video_controller widget into its own borderless window
            remove(video_controller);
            controller_window = std::make_unique<Gtk::Window>();
            controller_window->set_default_size(video.get_render_window_size().x, video_controller.get_height());
            controller_window->add(video_controller);
            controller_window->property_decorated().set_value(false);
            controller_window->set_title("Playback Controller");
            controller_window->show();

            //Re-parent the controller window to the render window, so that it shows on top
            auto parent_window_id = video.get_render_window_id();
            auto controller_window_id = GDK_WINDOW_XID(controller_window->get_window()->gobj());
            void *display = XOpenDisplay(nullptr);
            XReparentWindow(static_cast<Display *>(display), controller_window_id, parent_window_id, 0, video.get_render_window_size().y - video_controller.get_height());
            XMapRaised(static_cast<Display *>(display), controller_window_id);
            XFlush(static_cast<Display *>(display));
            XCloseDisplay(static_cast<Display *>(display));
            break;
        }
        case VideoWidget::FullscreenExited:
        {
            //Remove the video controller widget from it's own window and restore it to our own
            controller_window->remove();
            controller_window = nullptr;
            controller_hidden = false;
            add(video_controller);
            break;
        }
        case VideoWidget::MouseMoved:
        {
            last_cursor_move_time = std::chrono::system_clock::now();
            if(!controller_hidden || !controller_window)
                break;

            controller_window->show();
            controller_hidden = false;
            break;
        }
        case VideoWidget::Tick:
        {
            if(controller_hidden || !controller_window)
                break;
            if(last_cursor_move_time + std::chrono::seconds(3) < std::chrono::system_clock::now())
            {
                controller_window->hide();
                video.set_cursor_visible(false);
                controller_hidden = true;
            }
            break;
        }
        default:
            break;
    }
}