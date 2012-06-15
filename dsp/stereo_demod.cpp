/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
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
#include <gr_io_signature.h>
#include <gr_firdes.h>
#include <math.h>
#include <iostream>
#include <dsp/stereo_demod.h>


/* Create a new instance of stereo_demod and return a boost shared_ptr. */
stereo_demod_sptr make_stereo_demod(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new stereo_demod(quad_rate, audio_rate));
}


static const int MIN_IN  = 1; /* Mininum number of input streams. */
static const int MAX_IN  = 1; /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */


/*! \brief Create stereo demodulator object.
 *
 * Use make_stereo_demod() instead.
 */
stereo_demod::stereo_demod(float input_rate, float audio_rate)
    : gr_hier_block2("stereo_demod",
                     gr_make_io_signature (MIN_IN,  MAX_IN,  sizeof (float)),
                     gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (float))),
    d_input_rate(input_rate),
    d_audio_rate(audio_rate)
{
    //float gain;

    /* demodulator gain */
    //gain = 1.;
    //std::cout << "stereo_demod gain: " << gain << std::endl;

    // FIXME: cause we have no audio filter...
    std::cout << __FUNCTION__ << ": FIXME: audio filter" << std::endl;
    audio_rr0 = make_resampler_ff(d_audio_rate/d_input_rate);
    audio_rr1 = make_resampler_ff(d_audio_rate/d_input_rate);

    /* connect block */
    connect(self(), 0, audio_rr0, 0);
    connect(self(), 0, audio_rr1, 0);
    connect(audio_rr0, 0, self(), 0);
    connect(audio_rr1, 0, self(), 1);
}


stereo_demod::~stereo_demod()
{

}

