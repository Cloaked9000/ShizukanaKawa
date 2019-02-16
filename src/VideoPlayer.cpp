//
// Created by fred on 10/12/17.
//
#include <iostream>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/Graphics/Color.hpp>
#include <chrono>
#include <thread>
#include <Log.h>
#include <VideoPlayer.h>

#include "VideoPlayer.h"


extern "C" {
#include <X11/Xlib.h>
#include <X11/X.h>
#include <gdk/gdkx.h>
}

VideoPlayer::VideoPlayer(size_t parent_window_id_)
: audio_track_descriptions(nullptr),
  subtitle_track_descriptions(nullptr),
  current_audio_track_description(nullptr),
  current_subtitle_track_description(nullptr),
  parent_window_id(parent_window_id_),
  display(nullptr)
{
    XInitThreads();
    char const *vlc_argv[] = {"--no-xlib"};
    int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
    vlc = libvlc_new(vlc_argc, vlc_argv);
    if(!vlc)
        throw std::runtime_error("Failed to initialise libVLC");

    //Create a render window if we have a parent
    if(parent_window_id != 0)
    {
        window.create(sf::VideoMode(100, 100), "Render Window", 0);
        display = XOpenDisplay(nullptr);
        XReparentWindow(static_cast<Display *>(display), window.getSystemHandle(), parent_window_id, 0, 0);
        XMapRaised(static_cast<Display *>(display), window.getSystemHandle());
        XFlush(static_cast<Display *>(display));
    }
}


VideoPlayer::~VideoPlayer()
{
    if(audio_track_descriptions)
        libvlc_track_description_list_release(audio_track_descriptions);
    if(subtitle_track_descriptions)
        libvlc_track_description_list_release(subtitle_track_descriptions);
    if(display)
        XCloseDisplay(static_cast<Display *>(display));
    libvlc_release(vlc);
}

void VideoPlayer::open_from_stream(std::unique_ptr<sf::InputStream> stream, const std::vector<std::string> &vlc_options)
{
    player_context.stream = std::move(stream);
    player_context.media = libvlc_media_new_callbacks(vlc, open_callback, read_callback, seek_callback, close_callback, &player_context); //Take data from over SSH
    for(auto &option : vlc_options)
    {
        libvlc_media_add_option(player_context.media, option.c_str());
    }

    player_context.player = libvlc_media_player_new_from_media(player_context.media);
    if(parent_window_id != 0)
    {
        libvlc_media_player_set_xwindow(player_context.player, static_cast<uint32_t>(window.getSystemHandle()));
        libvlc_video_set_mouse_input(player_context.player, 0);
        libvlc_video_set_key_input(player_context.player, 0);
    }
    play();
}

int VideoPlayer::open_callback(void *opaque, void **datap, uint64_t *sizep)
{
    auto *ctx = static_cast<PlayerContext*>(opaque);
    *sizep = static_cast<uint64_t>(ctx->stream->getSize());
    *datap = ctx;
    return 0;
}

void VideoPlayer::close_callback(void * /*opaque */)
{

}

ssize_t VideoPlayer::read_callback(void *opaque, unsigned char *buf, size_t len)
{
    auto *ctx = static_cast<PlayerContext*>(opaque);
    return ctx->stream->read(buf, static_cast<sf::Int64>(len));
}

int VideoPlayer::seek_callback(void *opaque, uint64_t offset)
{
    auto *ctx = static_cast<PlayerContext*>(opaque);
    return ctx->stream->seek(static_cast<sf::Int64>(offset)) == -1 ? -1 : 0;
}

void VideoPlayer::set_position(const sf::Vector2i &pos)
{
    window.setPosition(pos);
}

void VideoPlayer::set_size(const sf::Vector2u &size)
{
    size_before_fullscreen = size;
    if(!libvlc_get_fullscreen(player_context.player))
        window.setSize(size);
}

VideoPlayer::State VideoPlayer::get_playback_state()
{
    if(libvlc_media_player_get_time(player_context.player) == -1)
        return State::Stopped;
    int is_playing = libvlc_media_player_is_playing(player_context.player) == 1;
    return is_playing ? State::Playing : State::Paused;
}

sf::Vector2u VideoPlayer::get_resolution() const
{
    unsigned x, y;
    libvlc_video_get_size(player_context.player, 0, &x, &y);
    return {x, y};
}

void VideoPlayer::set_playback_offset(size_t offset)
{
    libvlc_media_player_set_time(player_context.player, offset);
}

void VideoPlayer::toggle_pause()
{
    libvlc_media_player_pause(player_context.player);
}

ssize_t VideoPlayer::get_duration()
{
    return libvlc_media_player_get_length(player_context.player);
}

void VideoPlayer::save_screenshot(const std::string &filepath)
{
    const char *filepath_ptr = filepath.empty() ? nullptr : filepath.c_str();
    libvlc_video_take_snapshot(player_context.player, 0, filepath_ptr, 0, 0);
}

ssize_t VideoPlayer::get_playback_offset()
{
    ssize_t offset = libvlc_media_player_get_time(player_context.player);
    return offset == -1 ? 0u : static_cast<size_t>(offset);
}

void VideoPlayer::update()
{
    sf::Event e = {};
    while(window.pollEvent(e))
    {
        switch(e.type)
        {
            case sf::Event::KeyPressed:
                if(e.key.code == sf::Keyboard::F)
                    toggle_fullscreen();
                else if(e.key.code == sf::Keyboard::Space)
                    toggle_pause();
                else if(e.key.code == sf::Keyboard::B)
                    rotate_audio_track();
                else if(e.key.code == sf::Keyboard::V)
                    rotate_subtitle_track();
                else if(e.key.code == sf::Keyboard::S)
                    save_screenshot({});
                else if(e.key.code == sf::Keyboard::Escape)
                    toggle_fullscreen();
                break;
            case sf::Event::MouseMoved:
                mouse_moved(0, 0);
                break;
            default:
                break;
        }
    }

    window.clear(sf::Color::Black);
    window.display();
}

void VideoPlayer::play()
{
    libvlc_media_player_play(player_context.player);
}

void VideoPlayer::pause()
{
    libvlc_media_player_pause(player_context.player);
}

void VideoPlayer::toggle_fullscreen()
{
    //Toggle out of fullscreen
    Screen *screen = XScreenOfDisplay(static_cast<Display *>(display), 0);
    auto screen_count = XScreenNumberOfScreen(screen);
    Window root_win = RootWindow(static_cast<Display *>(display), screen_count);
    if(libvlc_get_fullscreen(player_context.player))
    {
        libvlc_set_fullscreen(player_context.player, false);
        XReparentWindow(static_cast<Display *>(display), window.getSystemHandle(), parent_window_id, 0, 0);
        XMapRaised(static_cast<Display *>(display), window.getSystemHandle());
        XFlush(static_cast<Display *>(display));
        window.setSize(size_before_fullscreen);
        return;
    }

    //Re-parent to root as only root can be fullscreen
    size_before_fullscreen = window.getSize();
    XReparentWindow(static_cast<Display *>(display), window.getSystemHandle(), root_win, 0, 0);
    XFlush(static_cast<Display *>(display));

    //Toggle into fullscreen
    libvlc_set_fullscreen(player_context.player, true);
}

void VideoPlayer::stop()
{
    if(player_context.player != nullptr)
    {
        libvlc_media_player_stop(player_context.player);
    }
}

void VideoPlayer::rotate_audio_track()
{
    //Load audio descriptions if not loaded
    if(!audio_track_descriptions)
    {
        //Load them
        audio_track_descriptions = libvlc_audio_get_track_description(player_context.player);
        if(!audio_track_descriptions)
            return;

        //Save the first one for going back to list head/freeing list
        current_audio_track_description = audio_track_descriptions;

        //Skip to current track
        int track_id = libvlc_audio_get_track(player_context.player);
        while(current_audio_track_description->i_id != track_id && current_audio_track_description->p_next != nullptr)
            current_audio_track_description = current_audio_track_description->p_next;
    }

    //Move to the next track
    if(current_audio_track_description->p_next == nullptr)
        current_audio_track_description = audio_track_descriptions;
    else
        current_audio_track_description = current_audio_track_description->p_next;

    libvlc_audio_set_track(player_context.player, current_audio_track_description->i_id);
    display_message("Track " + std::to_string(current_audio_track_description->i_id) + ": " + std::string(current_audio_track_description->psz_name));
}

void VideoPlayer::rotate_subtitle_track()
{
    //Load subtitle descriptions if not loaded
    if(!subtitle_track_descriptions)
    {
        //Load them
        subtitle_track_descriptions = libvlc_video_get_spu_description(player_context.player);
        if(!subtitle_track_descriptions)
            return;

        //Save the first one for going back to list head/freeing list
        current_subtitle_track_description = subtitle_track_descriptions;

        //Skip to current track
        int track_id = libvlc_video_get_spu(player_context.player);
        while(current_subtitle_track_description->i_id != track_id && current_subtitle_track_description->p_next != nullptr)
            current_subtitle_track_description = current_subtitle_track_description->p_next;
    }


    //Move to the next track
    if(current_subtitle_track_description->p_next == nullptr)
        current_subtitle_track_description = subtitle_track_descriptions;
    else
        current_subtitle_track_description = current_subtitle_track_description->p_next;

    libvlc_video_set_spu(player_context.player, current_subtitle_track_description->i_id);
    display_message("Subtitle " + std::to_string(current_subtitle_track_description->i_id) + ": " + std::string(current_subtitle_track_description->psz_name));
}

size_t VideoPlayer::get_render_window_id()
{
    return window.getSystemHandle();
}

bool VideoPlayer::is_fullscreen()
{
    return static_cast<bool>(libvlc_get_fullscreen(player_context.player));
}

sf::Vector2u VideoPlayer::get_render_window_size()
{
    sf::Vector2u ret;
    XWindowAttributes attr = {};
    XGetWindowAttributes((Display*)display, window.getSystemHandle(), &attr);
    ret.x = attr.width;
    ret.y = attr.height;
    return ret;
}

void VideoPlayer::display_message(const std::string &message)
{
    frlog << Log::info << message << Log::end;
    libvlc_video_set_marquee_int(player_context.player, libvlc_video_marquee_option_t::libvlc_marquee_Enable, true);
    libvlc_video_set_marquee_int(player_context.player, libvlc_video_marquee_option_t::libvlc_marquee_Timeout, 3200);
    libvlc_video_set_marquee_string(player_context.player, libvlc_video_marquee_option_t::libvlc_marquee_Text, message.c_str());
}

void VideoPlayer::set_cursor_visible(bool visible)
{
    window.setMouseCursorVisible(visible);
}


size_t VideoPlayer::get_subtitle_track()
{
    ssize_t sub_id = libvlc_video_get_spu(player_context.player);
    return sub_id < 0 ? 0u : static_cast<size_t>(sub_id);
}

size_t VideoPlayer::get_audio_track()
{
    ssize_t track_id = libvlc_audio_get_track(player_context.player);
    return track_id < 0 ? 0u : static_cast<size_t>(track_id);
}

void VideoPlayer::set_audio_track(size_t track_id)
{
    current_audio_track_description = nullptr;
    libvlc_audio_set_track(player_context.player, track_id);
}

void VideoPlayer::set_subtitle_track(size_t sub_id)
{
    current_subtitle_track_description = nullptr;
    libvlc_video_set_spu(player_context.player, sub_id);
}