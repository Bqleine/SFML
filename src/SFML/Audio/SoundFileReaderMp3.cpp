////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2022 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#define MINIMP3_IMPLEMENTATION // Minimp3 control define, places implementation in this file.
#ifndef NOMINMAX
#define NOMINMAX // To avoid windows.h and std::min issue
#endif
#define MINIMP3_NO_STDIO // Minimp3 control define, eliminate file manipulation code which is useless here

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4242 4244 4267 4456 4706)
#endif

#include <minimp3_ex.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef NOMINMAX
#undef MINIMP3_NO_STDIO

#include <SFML/Audio/SoundFileReaderMp3.hpp>
#include <SFML/System/InputStream.hpp>

#include <algorithm>
#include <cstring>


namespace
{
std::size_t readCallback(void* ptr, std::size_t size, void* data)
{
    sf::InputStream* stream = static_cast<sf::InputStream*>(data);
    return static_cast<std::size_t>(stream->read(ptr, static_cast<std::int64_t>(size)));
}

int seekCallback(std::uint64_t offset, void* data)
{
    sf::InputStream* stream   = static_cast<sf::InputStream*>(data);
    std::int64_t     position = stream->seek(static_cast<std::int64_t>(offset));
    return position < 0 ? -1 : 0;
}

bool hasValidId3Tag(const std::uint8_t* header)
{
    return std::memcmp(header, "ID3", 3) == 0 &&
           !((header[5] & 15) || (header[6] & 0x80) || (header[7] & 0x80) || (header[8] & 0x80) || (header[9] & 0x80));
}
} // namespace

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
bool SoundFileReaderMp3::check(InputStream& stream)
{
    std::uint8_t header[10];

    if (static_cast<std::size_t>(stream.read(header, static_cast<std::int64_t>(sizeof(header)))) < sizeof(header))
        return false;

    if (hasValidId3Tag(header))
        return true;

    if (hdr_valid(header))
        return true;

    return false;
}


////////////////////////////////////////////////////////////
SoundFileReaderMp3::SoundFileReaderMp3()
{
    m_io.read = readCallback;
    m_io.seek = seekCallback;
}


////////////////////////////////////////////////////////////
SoundFileReaderMp3::~SoundFileReaderMp3()
{
    mp3dec_ex_close(&m_decoder);
}


////////////////////////////////////////////////////////////
bool SoundFileReaderMp3::open(InputStream& stream, Info& info)
{
    // Init IO callbacks
    m_io.read_data = &stream;
    m_io.seek_data = &stream;

    // Init mp3 decoder
    mp3dec_ex_open_cb(&m_decoder, &m_io, MP3D_SEEK_TO_SAMPLE);
    if (!m_decoder.samples)
        return false;

    // Retrieve the music attributes
    info.channelCount = static_cast<unsigned int>(m_decoder.info.channels);
    info.sampleRate   = static_cast<unsigned int>(m_decoder.info.hz);
    info.sampleCount  = m_decoder.samples;

    m_numSamples = info.sampleCount;
    return true;
}


////////////////////////////////////////////////////////////
void SoundFileReaderMp3::seek(std::uint64_t sampleOffset)
{
    m_position = std::min(sampleOffset, m_numSamples);
    mp3dec_ex_seek(&m_decoder, m_position);
}


////////////////////////////////////////////////////////////
std::uint64_t SoundFileReaderMp3::read(std::int16_t* samples, std::uint64_t maxCount)
{
    std::uint64_t toRead = std::min(maxCount, m_numSamples - m_position);
    toRead = static_cast<std::uint64_t>(mp3dec_ex_read(&m_decoder, samples, static_cast<std::size_t>(toRead)));
    m_position += toRead;
    return toRead;
}

} // namespace priv

} // namespace sf
