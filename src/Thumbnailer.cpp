//
// Created by fred on 16/12/17.
//

#include "Thumbnailer.h"
#include <vlc/vlc.h>
#include <stdexcept>
#include <cstring>
#include <zconf.h>
#include <thread>
#include <iostream>

Thumbnailer::Thumbnailer()
{
    vlc = libvlc_new(0, nullptr);
    if(!vlc)
        throw std::runtime_error("Failed to initialise libVLC");
}

Thumbnailer::~Thumbnailer()
{
    libvlc_release(vlc);
}

sf::Image Thumbnailer::generate_thumbnail(sf::InputStream &stream, size_t width, size_t height)
{
    //Setup
    ThumbnailContext context;
    context.frame_data_size = width * height * 4;
    context.stream = &stream;
    context.frame_data = new uint8_t[context.frame_data_size];

    //Open the media from the stream, disabling audio/subtitles etc
    libvlc_media_t *media = libvlc_media_new_callbacks(vlc, open_callback, read_callback, seek_callback, close_callback, &context);
    libvlc_media_add_option(media, ":no-audio");
    libvlc_media_add_option(media, ":no-spu");
    libvlc_media_add_option(media, ":no-osd");


    //Render to an internal buffer
    libvlc_media_player_t *player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);
    libvlc_video_set_format(player, "RGBA", static_cast<unsigned int>(width), static_cast<unsigned int>(height), static_cast<unsigned int>(width * 4));
    libvlc_video_set_callbacks(player, lock_callback, unlock_callback, nullptr, &context);

    //Play media and seek to position
    libvlc_media_player_play(player);
    libvlc_media_player_set_position(player, 0.5f);
    const std::chrono::milliseconds wait_time(50);
    const uint32_t max_attempts = 200;
    for(uint32_t a = 0; a < max_attempts; ++a)
    {
        if(libvlc_media_player_is_playing(player) && libvlc_media_player_get_position(player) >= 0.5f)
            break;
        std::this_thread::sleep_for(wait_time);
    }
    context.seek_complete = true;

    //Wait for thumbnail to be generated
    for(uint32_t a = 0; a < max_attempts; ++a)
    {
        if(context.thumbnail_completed)
           break;
        std::this_thread::sleep_for(wait_time);
    }

    //Save it
    sf::Image thumbnail;
    thumbnail.create(static_cast<unsigned int>(width), static_cast<unsigned int>(height), context.frame_data);

    //Exit
    libvlc_media_player_release(player);
    delete[] context.frame_data;
    return thumbnail;
}

int Thumbnailer::open_callback(void *opaque, void **datap, uint64_t *sizep)
{
    auto *ctx = static_cast<ThumbnailContext*>(opaque);
    *sizep = static_cast<uint64_t>(ctx->stream->getSize());
    *datap = ctx;
    return 0;
}

void Thumbnailer::close_callback(void * /*opaque */)
{

}

ssize_t Thumbnailer::read_callback(void *opaque, unsigned char *buf, size_t len)
{
    auto *ctx = static_cast<ThumbnailContext*>(opaque);
    return ctx->stream->read(buf, static_cast<sf::Int64>(len));
}

int Thumbnailer::seek_callback (void *opaque, uint64_t offset)
{
    auto *ctx = static_cast<ThumbnailContext*>(opaque);
    return !ctx->stream->seek(static_cast<sf::Int64>(offset));
}

void *Thumbnailer::lock_callback(void *opaque, void **pixels)
{
    auto *context = static_cast<ThumbnailContext *>(opaque);
    *pixels = context->frame_data;
    return nullptr;
}

void Thumbnailer::unlock_callback(void *opaque, void * /*picture */, void *const * /*pixels */)
{
    auto *context = static_cast<ThumbnailContext *>(opaque);
    if(!context->seek_complete || context->thumbnail_completed)
        return;
    context->thumbnail_completed = true;
}

