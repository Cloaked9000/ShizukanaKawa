//
// Created by fred on 10/12/17.
//

#ifndef SFTPMEDIASTREAMER_VIDEOPLAYER_H
#define SFTPMEDIASTREAMER_VIDEOPLAYER_H


#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/InputStream.hpp>

extern "C"
{
#include <vlc/vlc.h>
}

class VideoPlayer
{
public:
    /*!
     * Constructs the video player
     *
     * @param parent_window_id The window ID of the parent, to
     * render into. Or 0 to render within a dedicated window.
     */
    explicit VideoPlayer(size_t parent_window_id);
    virtual ~VideoPlayer();

    enum State
    {
        Stopped = 0,
        Playing = 1,
        Paused = 2
    };

    /*!
     * Opens and starts playing a video from an input stream.
     *
     * @param stream The stream to read video data from
     * @param vlc_options Options to pass directly to LibVLC
     */
    virtual void open_from_stream(std::unique_ptr<sf::InputStream> stream, const std::vector<std::string> &vlc_options);

    /*!
     * Sets the position of the playback window,
     * relative to the parent window.
     *
     * @param pos The position to set the window to
     */
    virtual void set_position(const sf::Vector2i &pos);

    /*!
     * Sets the size of the playback window,
     * relative to the parent window.
     *
     * @param pos The size to set the window to
     */
    virtual void set_size(const sf::Vector2u &size);

    /*!
     * Gets the current playback state of the player (paused, playing, stopped etc)
     *
     * @return The current playback state
     */
    virtual State get_playback_state();

    /*!
     * Gets the video resolution, if loaded.
     *
     * @return The video resolution.
     */
    virtual sf::Vector2u get_resolution() const;

    /*!
     * Sets the current playback offset in milliseconds
     *
     * @param offset The new playback offset in milliseconds
     */
    virtual void set_playback_offset(size_t offset);

    /*!
     * Plays the video if paused, pauses the video
     * if playing.
     */
    virtual void toggle_pause();

    /*!
     * Gets the duration of the video being played, in milliseconds.
     *
     * @return The duration of the video being played, in milliseconds.
     */
    virtual ssize_t get_duration();

    /*!
     * Saves a snapshot of the current frame as an image to the requested filepath
     * as an image.
     *
     * @param filepath Where to store the screenshot. Pass an empty string if the default path should be used.
     */
    virtual void save_screenshot(const std::string &filepath);

    /*!
     * Gets the current playback offset in milliseconds
     *
     * @return The current playback offset in milliseconds
     */
    virtual ssize_t get_playback_offset();

    /*!
     * Called periodically internally
     * for event processing whilst in fullscreen.
     */
    virtual void update();

    /*!
     * Rotates the current audio track.
     * Can be called repeatedly to keep skipping
     * to the next track.
     */
    virtual void rotate_audio_track();

    /*!
     * Rotates the current subtitle track.
     * Can be called repeatedly to keep skipping
     * to the next track.
     */
    virtual void rotate_subtitle_track();

    /*!
     * Gets the window ID of the internal render window
     *
     * @return The ID of the internal render window
     */
    virtual size_t get_render_window_id();

    /*!
     * Gets the size of the internal render window
     *
     * @return The size of the internal render window
     */
    virtual sf::Vector2u get_render_window_size();

    /*!
     * Checks if the player is in fullscreen mode or not
     *
     * @return True if it is, false otherwise.
     */
    virtual bool is_fullscreen();

    /*!
     * Plays the video if paused
     */
    virtual void play();

    /*!
     * Pauses the video if playing
     */
    virtual void pause();

    /*!
     * Toggles between fullscreen mode
     * and windowed playback mode.
     */
    virtual void toggle_fullscreen();

    /*!
     * Stops and unloads the media currently being played.
     */
    virtual void stop();

    /*!
     * Displays a temporary message within the render window.
     *
     * @param message The message to display
     */
    virtual void display_message(const std::string &message);

    /*!
     * Sets if the cursor should be visible or invisible.
     *
     * @param visible True if the cursor should be visible, false otherwise.
     */
    virtual void set_cursor_visible(bool visible);

    /*!
     * Gets the ID of the current subtitle track
     *
     * @return ID of the current sub track
     */
    virtual size_t get_subtitle_track();

    /*!
     * Gets the ID of the current audio track
     *
     * @return The ID of the current audio track
     */
    virtual size_t get_audio_track();

    /*!
     * Sets the ID of the current audio track
     *
     * @param track_id ID of the audio track to use
     */
    virtual void set_audio_track(size_t track_id);

    /*!
     * Sets the ID of the current sub track
     *
     * @param sub_id The ID of the subtitle track to switch to
     */
    virtual void set_subtitle_track(size_t sub_id);

    /*!
     * Called internally for emitting events.
     *
     * @param x X of the mouse
     * @param y Y of the mouse
     */
    virtual void mouse_moved(size_t x, size_t y){(void)x; (void)y;};

private:
    VideoPlayer()= default;
    struct PlayerContext
    {
        PlayerContext()
        : media(nullptr),
          player(nullptr)
        {
        }
        ~PlayerContext()
        {
            libvlc_media_player_release(player);
            libvlc_media_release(media);
            player = nullptr;
            media = nullptr;
        }

        void operator=(const PlayerContext &o)=delete;
        PlayerContext(const PlayerContext &o)=delete;
        void operator=(PlayerContext &&o)=delete;
        PlayerContext(PlayerContext &&o)=delete;

        std::unique_ptr<sf::InputStream> stream;
        libvlc_media_t *media;
        libvlc_media_player_t *player;
    };

    //libVLC callbacks
    static int open_callback(void *opaque, void **datap, uint64_t *sizep);
    static void close_callback(void *opaque);
    static ssize_t read_callback(void *opaque, unsigned char *buf, size_t len);
    static int seek_callback(void *opaque, uint64_t offset);

    //State
    PlayerContext player_context;
    libvlc_instance_t *vlc;
    libvlc_track_description_t *audio_track_descriptions;
    libvlc_track_description_t *subtitle_track_descriptions;
    libvlc_track_description_t *current_audio_track_description;
    libvlc_track_description_t *current_subtitle_track_description;

    //Render window
    sf::RenderWindow window;
    size_t parent_window_id;
    void *display;
    sf::Vector2u size_before_fullscreen;
};


#endif //SFTPMEDIASTREAMER_VIDEOPLAYER_H
