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
#include <gnuradio/io_signature.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include <stdio.h>

#include "pa_sink.h"


/*! \brief Create a new pulseaudio sink object.
 *  \param device_name The name of the audio device, or NULL for default.
 *  \param audio_rate The sample rate of the audio stream.
 *  \param app_name Application name.
 *  \param stream_name The audio stream name.
 *
 * This is effectively the public constructor for pa_sink.
 */
pa_sink_sptr make_pa_sink(const string device_name, int audio_rate,
                          const string app_name, const string stream_name)
{
    return gnuradio::get_initial_sptr(new pa_sink(device_name, audio_rate, app_name, stream_name));
}


pa_sink::pa_sink(const string device_name, int audio_rate,
                 const string app_name, const string stream_name)
  : gr::sync_block ("pa_sink",
        gr::io_signature::make (1, 2, sizeof(float)),
        gr::io_signature::make (0, 0, 0)),
    d_stream_name(stream_name),
    d_app_name(app_name)
{
    int error;

    /* The sample type to use */
    d_ss.format = PA_SAMPLE_FLOAT32LE;
    d_ss.rate = audio_rate;
    d_ss.channels = 2;
    d_pasink = pa_simple_new(NULL,
                             d_app_name.c_str(),
                             PA_STREAM_PLAYBACK,
                             device_name.empty() ? NULL : device_name.c_str(),
                             d_stream_name.c_str(),
                             &d_ss,
                             NULL,
                             NULL,
                             &error);

    if (!d_pasink) {
        /** FIXME: Throw an exception **/
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }

}


pa_sink::~pa_sink()
{
    if (d_pasink) {
        pa_simple_free(d_pasink);
    }
}

bool pa_sink::start()
{
    return true;
}

bool pa_sink::stop()
{
    return true;
}


/*! \brief Select a new pulseaudio output device.
 *  \param device_name The name of the new output.
 */
void pa_sink::select_device(string device_name)
{
    int error;

    pa_simple_free(d_pasink);

    d_pasink = pa_simple_new(NULL,
                             d_app_name.c_str(),
                             PA_STREAM_PLAYBACK,
                             device_name.empty() ? NULL : device_name.c_str(),
                             d_stream_name.c_str(),
                             &d_ss,
                             NULL,
                             NULL,
                             &error);

    if (!d_pasink) {
        /** FIXME: Throw an exception **/
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }
}


#define BUFFER_SIZE 100000

int pa_sink::work (int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    static float audio_buffer[BUFFER_SIZE];
    float *ptr = &audio_buffer[0];
    int i, error;

    (void) output_items;

    if (noutput_items > BUFFER_SIZE/2)
        noutput_items = BUFFER_SIZE/2;

    if (input_items.size() == 2)
    {
        // two channels (stereo)
        const float *data_l = (const float*) input_items[0]; // left channel
        const float *data_r = (const float*) input_items[1]; // right channel
        for (i = noutput_items; i > 0; i--)
        {
            *ptr++ = *data_l++;
            *ptr++ = *data_r++;
        }
    }
    else
    {
        // assume 1 channel (mono)
        const float *data = (const float*) input_items[0];
        for (i = noutput_items; i > 0; i--)
        {
            float a = *data++;
            *ptr++ = a; // same data in left and right channel
            *ptr++ = a;
        }
    }

    if (pa_simple_write(d_pasink, audio_buffer, 2*noutput_items*sizeof(float), &error) < 0) { //!!!
        fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
    }

    return noutput_items;
}
