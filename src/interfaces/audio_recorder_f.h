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
#ifndef AUDIO_RECORDER_F_H
#define AUDIO_RECORDER_F_H

#include <gnuradio/sync_block.h>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

class audio_recorder_f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<audio_recorder_f> audio_recorder_f_sptr;
#else
typedef std::shared_ptr<audio_recorder_f> audio_recorder_f_sptr;
#endif

audio_recorder_f_sptr make_audio_recorder_f(int sample_rate);

class audio_recorder_f : public gr::sync_block
{
public:
    audio_recorder_f(int sample_rate);
    ~audio_recorder_f() override;

    bool start_recording(const std::string &filename, float gain);
    void stop_recording();
    void set_gain(float gain);
    bool is_recording() const;

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    void write_header();
    void update_header();

    mutable std::mutex d_mutex;
    int d_sample_rate;
    float d_gain;
    bool d_recording;
    FILE *d_fp;
    uint32_t d_bytes_written;
};

#endif // AUDIO_RECORDER_F_H
