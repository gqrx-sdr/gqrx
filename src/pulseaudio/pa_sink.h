/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#ifndef PA_SINK_H
#define PA_SINK_H

#include <string>
#include <gnuradio/sync_block.h>
#include <gnuradio/high_res_timer.h>
#include <pulse/simple.h>

using namespace std;

class pa_sink;

typedef boost::shared_ptr<pa_sink> pa_sink_sptr;

pa_sink_sptr make_pa_sink(const string device_name, int audio_rate,
                          const string app_name="GNU Radio",
                          const string stream_name="SDR");


/*! \brief Pulseaudio sink
 *  \ingroup IO
 *
 * This block implements a two-channel pulseaudio sink using the Pulseaudio simple API.
 *
 */
class pa_sink : public gr::sync_block
{
    friend pa_sink_sptr make_pa_sink(const string device_name, int audio_rate,
                                     const string app_name, const string stream_name);

public:
    pa_sink(const string device_name, int audio_rate,
            const string app_name="GNU Radio", const string stream_name="SDR");
    ~pa_sink();

    int work (int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items);

    bool start();
    bool stop();

    void select_device(string device_name);

private:
    pa_simple *d_pasink;    /*! The pulseaudio object. */
    string d_stream_name;   /*! Descriptive name of the stream. */
    string d_app_name;      /*! Descriptive name of the applcation. */
    pa_sample_spec d_ss;    /*! pulseaudio sample specification. */
    pa_buffer_attr d_attr;  /*! Buffer attributes. */

    // FIXME
    // periodic flushing of audio buffer (until we have soundcard calibration)
    int d_auto_flush;  // flush interval in seconds (negative means off)
    gr::high_res_timer_type d_last_flush;
};

#endif /* PA_SINK_H */
