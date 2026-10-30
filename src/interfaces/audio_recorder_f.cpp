/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Alexandru Csete OZ9AEC and Gqrx contributors.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include "audio_recorder_f.h"
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cmath>

audio_recorder_f_sptr make_audio_recorder_f(int sample_rate)
{
    return gnuradio::get_initial_sptr(new audio_recorder_f(sample_rate));
}

static void write_u16_le(FILE *fp, uint16_t val)
{
    uint8_t b[2];
    b[0] = static_cast<uint8_t>(val & 0xFF);
    b[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    fwrite(b, 1, 2, fp);
}

static void write_u32_le(FILE *fp, uint32_t val)
{
    uint8_t b[4];
    b[0] = static_cast<uint8_t>(val & 0xFF);
    b[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    b[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    b[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
    fwrite(b, 1, 4, fp);
}

audio_recorder_f::audio_recorder_f(int sample_rate)
    : gr::sync_block("audio_recorder_f",
                     gr::io_signature::make(2, 2, sizeof(float)),
                     gr::io_signature::make(0, 0, 0)),
      d_sample_rate(sample_rate),
      d_gain(1.0f),
      d_recording(false),
      d_fp(nullptr),
      d_bytes_written(0)
{
}

audio_recorder_f::~audio_recorder_f()
{
    stop_recording();
}

void audio_recorder_f::write_header()
{
    if (!d_fp)
        return;

    // RIFF header
    fwrite("RIFF", 1, 4, d_fp);
    write_u32_le(d_fp, 36); // placeholder for chunk size
    fwrite("WAVE", 1, 4, d_fp);

    // "fmt " subchunk
    fwrite("fmt ", 1, 4, d_fp);
    write_u32_le(d_fp, 16); // Subchunk1Size (16 for PCM)
    write_u16_le(d_fp, 1);  // AudioFormat (1 for PCM)
    write_u16_le(d_fp, 2);  // NumChannels (stereo)
    write_u32_le(d_fp, static_cast<uint32_t>(d_sample_rate)); // SampleRate
    write_u32_le(d_fp, static_cast<uint32_t>(d_sample_rate * 2 * sizeof(int16_t))); // ByteRate
    write_u16_le(d_fp, static_cast<uint16_t>(2 * sizeof(int16_t))); // BlockAlign
    write_u16_le(d_fp, 16); // BitsPerSample

    // "data" subchunk
    fwrite("data", 1, 4, d_fp);
    write_u32_le(d_fp, 0);  // placeholder for subchunk2 size
}

void audio_recorder_f::update_header()
{
    if (!d_fp)
        return;

    // Update RIFF chunk size: 36 + subchunk2 size
    fseek(d_fp, 4, SEEK_SET);
    write_u32_le(d_fp, 36 + d_bytes_written);

    // Update data subchunk size
    fseek(d_fp, 40, SEEK_SET);
    write_u32_le(d_fp, d_bytes_written);
}

bool audio_recorder_f::start_recording(const std::string &filename, float gain)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    if (d_recording || filename.empty())
        return false;

    if (std::isnan(gain) || std::isinf(gain))
        gain = 0.0f;

    d_fp = fopen(filename.c_str(), "wb");
    if (!d_fp)
        return false;

    d_gain = gain;
    d_bytes_written = 0;
    write_header();
    d_recording = true;
    return true;
}

void audio_recorder_f::stop_recording()
{
    std::lock_guard<std::mutex> lock(d_mutex);
    if (!d_recording || !d_fp)
        return;

    d_recording = false;
    update_header();
    fclose(d_fp);
    d_fp = nullptr;
}

void audio_recorder_f::set_gain(float gain)
{
    std::lock_guard<std::mutex> lock(d_mutex);
    if (std::isnan(gain) || std::isinf(gain))
        gain = 0.0f;
    d_gain = gain;
}

bool audio_recorder_f::is_recording() const
{
    std::lock_guard<std::mutex> lock(d_mutex);
    return d_recording;
}

int audio_recorder_f::work(int noutput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
{
    (void)output_items;
    const float *in0 = static_cast<const float *>(input_items[0]);
    const float *in1 = static_cast<const float *>(input_items[1]);

    std::lock_guard<std::mutex> lock(d_mutex);
    if (!d_recording || !d_fp)
        return noutput_items;

    std::vector<uint8_t> buf(noutput_items * 4);
    for (int i = 0; i < noutput_items; ++i)
    {
        float val0 = in0[i] * d_gain;
        float val1 = in1[i] * d_gain;

        if (std::isnan(val0) || std::isinf(val0))
            val0 = 0.0f;
        if (std::isnan(val1) || std::isinf(val1))
            val1 = 0.0f;

        val0 = std::clamp(val0 * 32767.0f, -32768.0f, 32767.0f);
        val1 = std::clamp(val1 * 32767.0f, -32768.0f, 32767.0f);

        int16_t s0 = static_cast<int16_t>(std::round(val0));
        int16_t s1 = static_cast<int16_t>(std::round(val1));

        buf[4 * i]     = static_cast<uint8_t>(s0 & 0xFF);
        buf[4 * i + 1] = static_cast<uint8_t>((s0 >> 8) & 0xFF);
        buf[4 * i + 2] = static_cast<uint8_t>(s1 & 0xFF);
        buf[4 * i + 3] = static_cast<uint8_t>((s1 >> 8) & 0xFF);
    }

    size_t written = fwrite(buf.data(), 1, buf.size(), d_fp);
    d_bytes_written += static_cast<uint32_t>(written);

    return noutput_items;
}
