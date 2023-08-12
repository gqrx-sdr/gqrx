/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#include <gnuradio/io_signature.h>
#include <math.h>
#include <iostream>
#include <dsp/stereo_demod.h>


/* Create a new instance of stereo_demod and return a shared_ptr. */
stereo_demod_sptr make_stereo_demod(float quad_rate, float audio_rate,
                                    bool stereo, bool oirt)
{
    return gnuradio::get_initial_sptr(new stereo_demod(quad_rate,
                                                       audio_rate, stereo, oirt));
}


static const int MIN_IN  = 1; /* Minimum number of input streams. */
static const int MAX_IN  = 1; /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

/*! \brief Create stereo demodulator object.
 *
 * Use make_stereo_demod() instead.
 */
stereo_demod::stereo_demod(float input_rate, float audio_rate, bool stereo, bool oirt)
    : gr::hier_block2("stereo_demod",
                     gr::io_signature::make (MIN_IN,  MAX_IN,  sizeof (float)),
                     gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_input_rate(input_rate),
    d_audio_rate(audio_rate),
    d_stereo(stereo),
    d_oirt(oirt)
{
  double cutof_freq = d_oirt ? 15e3 : 17e3;
  lpf0 = make_lpf_ff((double)d_input_rate, cutof_freq, 2e3); // FIXME
  audio_rr0 = make_resampler_ff(d_audio_rate/d_input_rate);
  deemph0 = make_fm_deemph(d_audio_rate, 50.0e-6);

  if (d_stereo)
  {
    lpf1 = make_lpf_ff((double)d_input_rate, cutof_freq, 2e3, -2.1); // FIXME
    audio_rr1 = make_resampler_ff(d_audio_rate/d_input_rate);
    deemph1 = make_fm_deemph(d_audio_rate, 50.0e-6);

    if (!d_oirt)
    {
        d_tone_taps = gr::filter::firdes::complex_band_pass(
                                       1.0,                  // gain,
                                       (double)d_input_rate, // sampling_freq
                                       18980.,               // low_cutoff_freq
                                       19020.,               // high_cutoff_freq
                                       5000.);               // transition_width
        pll = gr::analog::pll_refout_cc::make(0.0002,    // loop_bw FIXME
                                2 * (float)M_PI * 19020 / input_rate,  // max_freq
                                2 * (float)M_PI * 18980 / input_rate); // min_freq
        subtone = gr::blocks::multiply_cc::make();
    } else {
        d_tone_taps = gr::filter::firdes::complex_band_pass(
                                       1.0,                  // gain,
                                       (double)d_input_rate, // sampling_freq
                                       31200.,               // low_cutoff_freq
                                       31300.,               // high_cutoff_freq
                                       5000.);               // transition_width
        pll = gr::analog::pll_refout_cc::make(0.0002,    // loop_bw FIXME
                                2 * (float)M_PI * 31200 / input_rate,  // max_freq
                                2 * (float)M_PI * 31300 / input_rate); // min_freq
    }

    tone = gr::filter::fir_filter_fcc::make(1, d_tone_taps);
    delay = gr::blocks::delay::make(sizeof(float), (d_tone_taps.size() - 1) / 2);

    lo = gr::blocks::complex_to_imag::make();

    mixer = gr::blocks::multiply_ff::make();

    add = gr::blocks::add_ff::make();
    sub = gr::blocks::sub_ff::make();

    /* connect block */
    if (!d_oirt) {
        connect(self(), 0, tone, 0);
        connect(self(), 0, delay, 0);
        connect(tone, 0, pll, 0);
        connect(pll, 0, subtone, 0);
        connect(pll, 0, subtone, 1);
        connect(subtone, 0, lo, 0);

        connect(lo, 0, mixer, 0);
    } else {
        connect(self(), 0, tone, 0);
        connect(self(), 0, delay, 0);
        connect(tone, 0, pll, 0);
        connect(pll, 0, lo, 0);
        connect(lo, 0, mixer, 0);
    }

    connect(delay, 0, mixer, 1);

    connect(delay, 0, lpf0, 0);
    connect(mixer, 0, lpf1, 0);

    connect(lpf0, 0, audio_rr0, 0); // sum
    connect(lpf1, 0, audio_rr1, 0); // delta

    connect(audio_rr0, 0, add,   0);
    connect(audio_rr1, 0, add,   1);
    connect(add,       0, deemph0, 0);
    connect(deemph0,   0, self(), 0); // left = sum + delta

    connect(audio_rr0, 0, sub,   0);
    connect(audio_rr1, 0, sub,   1);
    connect(sub,       0, deemph1, 0);
    connect(deemph1,   0, self(), 1); // right = sum - delta
  }
  else // if (!d_stereo)
  {
    /* connect block */
    connect(self(), 0, lpf0, 0);
    connect(lpf0,   0, audio_rr0, 0);
    connect(audio_rr0, 0, deemph0, 0);
    connect(deemph0, 0, self(), 0);
    connect(deemph0, 0, self(), 1);
  }
}


stereo_demod::~stereo_demod()
{

}

void stereo_demod::set_audio_rate(float audio_rate)
{
    if (std::abs(d_audio_rate-audio_rate) > 0.5f)
    {
        d_audio_rate = audio_rate;
        audio_rr0->set_rate(d_audio_rate/d_input_rate);
        deemph0->set_rate(d_audio_rate);
        if(d_stereo)
        {
            audio_rr1->set_rate(d_audio_rate/d_input_rate);
            deemph1->set_rate(d_audio_rate);
        }
    }
}

