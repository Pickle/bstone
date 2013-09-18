//
// A decoder for AdLib music.
//


#include "bstone_adlib_music_decoder.h"

#include "SDL.h"


namespace {
} // namespace


namespace bstone {


AdlibMusicDecoder::AdlibMusicDecoder() :
    commands_count_(0),
    command_index_(0),
    samples_per_tick_(0),
    remains_count_(0)
{
}

// (virtual)
bool AdlibMusicDecoder::initialize(
    const void* raw_data,
    int raw_size,
    int dst_rate)
{
    if (!AdlibDecoder::initialize(
        raw_data,
        raw_size,
        dst_rate))
    {
        return false;
    }

    reader_.initialize(raw_data, raw_size);

    int commands_size = SDL_SwapLE16(reader_.read_u16());

    if ((commands_size % 4) != 0)
        return false;

    if ((commands_size + 2) > raw_size)
        return false;

    command_index_ = 0;
    commands_count_ = commands_size / 4;

    samples_per_tick_ =  emulator_.get_sample_rate() / get_tick_rate();
    remains_count_ = 0;

    int ticks_count = 0;

    for (int i = 0; i < commands_count_; ++i) {
        reader_.skip(2);
        ticks_count += reader_.read_u16();
    }

    set_dst_length_in_samples(ticks_count * samples_per_tick_);

    reader_.set_position(2);

    set_is_initialized(true);

    return true;
}

// (virtual)
void AdlibMusicDecoder::uninitialize()
{
    reader_.uninitialize();
    commands_count_ = 0;
    command_index_ = 0;
    samples_per_tick_ = 0;
    remains_count_ = 0;

    AdlibDecoder::uninitialize();
}

// (virtual)
bool AdlibMusicDecoder::reset()
{
    if (!AdlibDecoder::reset())
        return false;

    reader_.set_position(2);

    command_index_ = 0;
    remains_count_ = 0;

    return true;
}

// (virtual)
int AdlibMusicDecoder::decode(
    int dst_count,
    int16_t* dst_data)
{
    if (!is_initialized())
        return 0;

    if (dst_count < 1)
        return 0;

    if (dst_data == NULL)
        return 0;

    if (command_index_ == commands_count_ && remains_count_ == 0)
        return 0;

    int decoded_samples_count = 0;

    for (bool quit = false; !quit; )
    {
        if (remains_count_ > 0) {
            int count = std::min(dst_count, remains_count_);

            emulator_.generate(count, dst_data);

            dst_data += count;
            dst_count -= count;
            remains_count_ -= count;
            decoded_samples_count += count;
        } else {
            int delay = 0;

            while (command_index_ < commands_count_ && delay == 0) {
                int command_port = reader_.read_u8();
                int command_value = reader_.read_u8();
                delay = reader_.read_u16();

                emulator_.write(command_port, command_value);
                ++command_index_;
            }

            if (delay > 0)
                remains_count_ = delay * samples_per_tick_;
        }

        quit =
            (command_index_ == commands_count_ && remains_count_ == 0) ||
            dst_count == 0;
    }

    return decoded_samples_count;
}

// (static)
int AdlibMusicDecoder::get_tick_rate()
{
    return 700;
}


} // namespace bstone