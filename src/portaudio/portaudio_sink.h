/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2016 Alexandru Csete OZ9AEC.
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
#pragma once

#include <gnuradio/sync_block.h>
#include <portaudio.h>
#include <string>

using namespace std;

class portaudio_sink;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<portaudio_sink> portaudio_sink_sptr;
#else
typedef std::shared_ptr<portaudio_sink> portaudio_sink_sptr;
#endif

portaudio_sink_sptr make_portaudio_sink(const string device_name, int audio_rate,
                                        const string app_name);

#define BUFFER_SIZE 100000

/** Two-channel portaudio sink */
class portaudio_sink : public gr::sync_block
{
    friend portaudio_sink_sptr make_portaudio_sink(const string device_name,
                                                   int audio_rate,
                                                   const string app_name);

public:
    portaudio_sink(const string device_name, int audio_rate, const string app_name);
    ~portaudio_sink();

    int work (int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

    bool start();
    bool stop();

    void select_device(string device_name, int audio_rate, string /* stream_name */);

private:
    void open_stream();
    void close_stream();

    PaStream           *d_stream;
    PaStreamParameters  d_out_params;
    string              d_app_name;          // Descriptive name of the application.
    int                 d_audio_rate;

    float    audio_buffer[BUFFER_SIZE];
};
