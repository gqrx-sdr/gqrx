/* -*- c++ -*- */
/*
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
#ifndef STEREO_DEMOD_H
#define STEREO_DEMOD_H

#include <gr_hier_block2.h>
#include <gr_firdes.h>
#include <gr_fir_filter_fcc.h>
#include <gr_fir_filter_fff.h>
#include <gr_pll_refout_cc.h>
#include <gr_multiply_cc.h>
#include <gr_multiply_ff.h>
#include <gr_multiply_const_ff.h>
#include <gr_complex_to_xxx.h>
#include <gr_add_ff.h>
#include <vector>
#include "dsp/lpf.h"
#include "dsp/resampler_xx.h"

 
class stereo_demod;

typedef boost::shared_ptr<stereo_demod> stereo_demod_sptr;


/*! \brief Return a shared_ptr to a new instance of stere_demod.
 *  \param quad_rate The input sample rate.
 *  \param audio_rate The audio rate.
 *  \param stereo On/off stereo mode.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, stereo_demod's constructor is private.
 * make_stereo_demod is the public interface for creating new instances.
 */
stereo_demod_sptr make_stereo_demod(float quad_rate=120e3,
                                    float audio_rate=48e3,
                                    bool stereo=true);


/*! \brief FM stereo demodulator.
 *  \ingroup DSP
 *
 * This class implements the stereo demodulator for 87.5...108 MHz band.
 *
 */
class stereo_demod : public gr_hier_block2
{
    friend stereo_demod_sptr make_stereo_demod(float input_rate,
                                               float audio_rate,
                                               bool stereo);

protected:
    stereo_demod(float input_rate, float audio_rate, bool stereo);

public:
    ~stereo_demod();

private:
    /* GR blocks */
    gr_fir_filter_fcc_sptr  tone;  /*!< Pilot tone BPF. */
    gr_pll_refout_cc_sptr   pll;   /*!< Pilot tone PLL. */
    gr_multiply_cc_sptr subtone;   /*!< Stereo subtone. */
    gr_complex_to_imag_sptr lo;    /*!< Complex tone imag. */
    gr_fir_filter_fff_sptr  lo2;   /*!< Subtone BPF. */
    gr_multiply_ff_sptr mixer;     /*!< Balance mixer. */
    lpf_ff_sptr lpf0;              /*!< Low-pass filter #0. */
    lpf_ff_sptr lpf1;              /*!< Low-pass filter #1. */
    resampler_ff_sptr audio_rr0;   /*!< Audio resampler #0. */
    resampler_ff_sptr audio_rr1;   /*!< Audio resampler #1. */
    gr_multiply_const_ff_sptr cdp; /*!< Channel delta (plus). */
    gr_multiply_const_ff_sptr cdm; /*!< Channel delta (minus). */
    gr_add_ff_sptr add0;           /*!< Left stereo channel. */
    gr_add_ff_sptr add1;           /*!< Right stereo channel. */

    /* other parameters */
    bool  d_stereo;                      /*! On/off stereo mode. */
    float d_input_rate;                  /*! Input rate. */
    float d_audio_rate;                  /*! Audio rate. */
    std::vector<gr_complex> d_tone_taps; /*! Tone BPF taps. */
    std::vector<float> d_pll_taps;       /*! Subtone BPF taps. */
};


#endif // STEREO_DEMOD_H
