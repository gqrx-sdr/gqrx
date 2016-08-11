/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#include <gnuradio/io_signature.h>
#include <stdio.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include "pa_source.h"


#define SAMPLES_PER_BUFFER 1024 /*! Max number of samples we read every cycle. */


/*! \brief Create a new pulseaudio source object.
 *  \param device_name The name of the audio device, or NULL for default.
 *  \param audio_rate The sample rate of the audio stream.
 *  \param num_chan The number of channels (1 for mono, 2 for stereo)
 *  \param app_name Application name.
 *  \param stream_name The audio stream name.
 *
 * This is effectively the public constructor for pa_source.
 */
pa_source_sptr make_pa_source(const string device_name, int sample_rate, int num_chan,
                              const string app_name, const string stream_name)
{
    return gnuradio::get_initial_sptr (new pa_source (device_name, sample_rate, num_chan, app_name, stream_name));
}


pa_source::pa_source (const string device_name, int sample_rate, int num_chan,
                      const string app_name, const string stream_name)
  : gr::sync_block ("pa_source",
        gr::io_signature::make (0, 0, 0),
        gr::io_signature::make (0, 0, 0)),
    d_stream_name(stream_name),
    d_app_name(app_name)
{
    int error;

/** FIXME: only 2 channels supported **/
//    if ((num_chan != 1) && (num_chan != 2)) {
//        num_chan = 2;
//    }
    num_chan = 2;

    set_output_signature(gr::io_signature::make (1, num_chan, sizeof(float)));

    /* The sample type to use */
    d_ss.format = PA_SAMPLE_FLOAT32LE;
    d_ss.rate = sample_rate;
    d_ss.channels = num_chan;

    d_pasrc = pa_simple_new(NULL,
                            d_app_name.c_str(),
                            PA_STREAM_RECORD,
                            device_name.empty() ? NULL : device_name.c_str(),
                            d_stream_name.c_str(),
                            &d_ss,
                            NULL,
                            NULL,
                            &error);

    if (!d_pasrc) {
        /** FIXME: throw and exception */
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }
}


pa_source::~pa_source()
{
    if (d_pasrc) {
        pa_simple_free(d_pasrc);
    }
}


/*! \brief Select a new pulseaudio input device.
 *  \param device_name The name of the new output.
 */
void pa_source::select_device(string device_name)
{
    int error;

    pa_simple_free(d_pasrc);

    d_pasrc = pa_simple_new(NULL,
                            d_app_name.c_str(),
                            PA_STREAM_RECORD,
                            device_name.empty() ? NULL : device_name.c_str(),
                            d_stream_name.c_str(),
                            &d_ss,
                            NULL,
                            NULL,
                            &error);

    if (!d_pasrc) {
        /** FIXME: Throw an exception **/
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }
}

int pa_source::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    (void) input_items;

    float audio_buffer[SAMPLES_PER_BUFFER*2]; /** FIXME: Channels **/
    float *out0 = (float *) output_items[0];
    float *out1 = (float *) output_items[1];  // see gr_complex_to_float
    int error=0;
    int i=0;


    if (pa_simple_read(d_pasrc, &audio_buffer[0], sizeof(audio_buffer), &error) < 0)
    {
        fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));

        return 0;
    }

    for (i = 0; i < SAMPLES_PER_BUFFER; i++)
    {
        out0[i] = audio_buffer[i*2];
        out1[i] = audio_buffer[(i*2)+1];
    }

    return SAMPLES_PER_BUFFER;
}
