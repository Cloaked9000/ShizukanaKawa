//
// Created by fred on 31/12/17.
//

#ifndef SFTPMEDIASTREAMER_VIDEOCONTROLWIDGET_H
#define SFTPMEDIASTREAMER_VIDEOCONTROLWIDGET_H


#include <gtkmm/container.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include "VideoWidget.h"

class VideoControlWidget : public Gtk::Box
{
public:
    explicit VideoControlWidget(std::shared_ptr<VideoWidget> video);
    ~VideoControlWidget() override;

private:
    //Members
    bool update_seek_bar();
    bool pause_button_click_callback(GdkEventButton *button);
    bool stop_button_click_callback(GdkEventButton *button);
    bool seek_bar_click_callback(GdkEventButton *event);

    std::string seconds_to_text(size_t seconds);


    //State
    sigc::connection seek_bar_update_clock;
    Gtk::EventBox pause_button_box;
    Gtk::Image pause_button;

    Gtk::EventBox stop_button_box;
    Gtk::Image stop_button;

    Gtk::ProgressBar seek_bar;
    Gtk::EventBox seek_bar_box;
    Gtk::Box button_control_box;

    Gtk::Label video_offset_label;
    Gtk::Label video_duration_label;

    Glib::RefPtr<Gdk::Pixbuf> pause_icon;
    Glib::RefPtr<Gdk::Pixbuf> play_icon;
    Glib::RefPtr<Gdk::Pixbuf> stop_icon;

    //Signal handlers
    sigc::connection pause_button_signal;
    sigc::connection stop_button_signal;
    sigc::connection seek_bar_signal;

    //Dependencies
    std::shared_ptr<VideoWidget> video;
};


#endif //SFTPMEDIASTREAMER_VIDEOCONTROLWIDGET_H
