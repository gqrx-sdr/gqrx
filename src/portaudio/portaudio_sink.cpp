/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#include <gnuradio/io_signature.h>
#include <stdio.h>

#include "device_list.h"
#include "portaudio_sink.h"

/**
 * Create a new portaudio sink object.
 * @param device_name The name of the audio device, or NULL for default.
 * @param audio_rate The sample rate of the audio stream.
 * @param app_name Application name.
 * @param stream_name The audio stream name.
 */
portaudio_sink_sptr make_portaudio_sink(const string device_name,
                                        int audio_rate,
                                        const string app_name,
                                        const string stream_name)
{
    return gnuradio::get_initial_sptr(new portaudio_sink(device_name,
                                                         audio_rate,
                                                         app_name,
                                                         stream_name));
}

portaudio_sink::portaudio_sink(const string device_name, int audio_rate,
                 const string app_name, const string stream_name)
  : gr::sync_block ("portaudio_sink",
        gr::io_signature::make (1, 2, sizeof(float)),
        gr::io_signature::make (0, 0, 0)),
    d_stream_name(stream_name),
    d_app_name(app_name),
    d_audio_rate(audio_rate)
{

    // find device index
    PaDeviceIndex           idx;
    portaudio_device_list   devices;

    idx = devices.get_output_device_index(device_name);
    if (idx == -1)
    {
        fprintf(stderr, "Using default audio device\n");
        idx = Pa_GetDefaultOutputDevice();
    }
    else
    {
        fprintf(stderr, "Audio device '%s' has index %d\n",
                device_name.data(), idx);
    }

    // Initialize stream parmaeters
    d_out_params.device = idx;
    d_out_params.channelCount = 2;
    d_out_params.sampleFormat = paFloat32;
    d_out_params.suggestedLatency =
            Pa_GetDeviceInfo(d_out_params.device)->defaultHighOutputLatency;
    d_out_params.hostApiSpecificStreamInfo = NULL;

    if (Pa_IsFormatSupported(NULL, &d_out_params, d_audio_rate) != paFormatIsSupported)
        fprintf(stderr, "portaudio_sink(): Audio output device does not support requested format.\n");
}

portaudio_sink::~portaudio_sink()
{
    // nothing to do
}

/* open and start audio stream */
bool portaudio_sink::start()
{
    PaError     err;

    err = Pa_OpenStream(&d_stream,
                        NULL,           // inputParameters
                        &d_out_params,
                        d_audio_rate,
                        paFramesPerBufferUnspecified,
                        paClipOff,
                        NULL,           // no callback, use blocking API
                        NULL);

    if (err != paNoError)
    {
        fprintf(stderr,
                "portaudio_sink::start(): Failed to open audio stream: %s\n",
                Pa_GetErrorText(err));
        return false;
    }

    err = Pa_StartStream(d_stream);
    if (err != paNoError)
    {
        fprintf(stderr,
                "portaudio_sink::start(): Failed to start audio stream: %s\n",
                Pa_GetErrorText(err));
        return false;
    }

    return true;
}

/* Stop and close audio stream */
bool portaudio_sink::stop()
{
    PaError     err;
    bool        retval = true;

    err = Pa_StopStream(d_stream);
    if (err != paNoError)
    {
        retval = false;
        fprintf(stderr,
                "portaudio_sink::stop(): Error stopping audio stream: %s\n",
                Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(d_stream);
    if (err != paNoError)
    {
        retval = false;
        fprintf(stderr,
                "portaudio_sink::stop(): Error closing audio stream: %s\n",
                Pa_GetErrorText(err));
    }

    return retval;
}

void portaudio_sink::select_device(string device_name)
{

}

#define BUFFER_SIZE 100000
int portaudio_sink::work(int noutput_items,
                         gr_vector_const_void_star &input_items,
                         gr_vector_void_star &output_items)
{
    PaError     err;

    static float    audio_buffer[BUFFER_SIZE];
    float      *ptr = &audio_buffer[0];
    int         i;

    (void) output_items;

    if (noutput_items > BUFFER_SIZE/2)
        noutput_items = BUFFER_SIZE/2;

    // two channels (stereo)
    const float *data_l = (const float*) input_items[0];
    const float *data_r = (const float*) input_items[1];
    for (i = noutput_items; i > 0; i--)
    {
        *ptr++ = *data_l++;
        *ptr++ = *data_r++;
    }

    err = Pa_WriteStream(d_stream, audio_buffer, noutput_items);
    if (err)
        fprintf(stderr,
                "portaudio_sink::work(): Error writing to audio device: %s\n",
                Pa_GetErrorText(err));

    return noutput_items;

// code below supports 1 or 2 channels
#if 0
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
#endif

}
