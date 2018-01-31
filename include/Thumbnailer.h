//
// Created by fred on 16/12/17.
//

#ifndef SFTPMEDIASTREAMER_THUMBNAILER_H
#define SFTPMEDIASTREAMER_THUMBNAILER_H


#include <SFML/Graphics/Image.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <vlc/vlc.h>
#include <atomic>


class Thumbnailer
{
public:
    Thumbnailer();
    ~Thumbnailer();

    /*!
     * Generates a thumbnail for a given sf::InputStream.
     *
     * @throws An std::exception on failure.
     * @param stream The steam to generate the thumbnail from
     * @param width The width of the generated image
     * @param height The height of the generated image
     * @return The generated image on success.
     */
    sf::Image generate_thumbnail(sf::InputStream &stream, size_t width, size_t height);

private:
    //Generation context
    struct ThumbnailContext
    {
        ThumbnailContext()
        : frame_data(nullptr),
          frame_data_size(0),
          stream(nullptr),
          seek_complete(false),
          thumbnail_completed(false)
        {

        }

        uint8_t *frame_data;
        size_t frame_data_size;
        sf::InputStream *stream;
        std::atomic_bool seek_complete;
        std::atomic_bool thumbnail_completed;
    };

    //libVLC callbacks
    static int open_callback(void *opaque, void **datap, uint64_t *sizep);
    static void close_callback(void *opaque);
    static ssize_t read_callback(void *opaque, unsigned char *buf, size_t len);
    static int seek_callback(void *opaque, uint64_t offset);
    static void *lock_callback(void *opaque, void **pixels);
    static void unlock_callback(void *opaque, void *picture, void *const *pixels);

    libvlc_instance_t *vlc;
};


#endif //SFTPMEDIASTREAMER_THUMBNAILER_H
