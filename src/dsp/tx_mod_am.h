/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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

#include <gnuradio/analog/rail_ff.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/hier_block2.h>

class tx_mod_am;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_am> tx_mod_am_sptr;
#else
typedef std::shared_ptr<tx_mod_am> tx_mod_am_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of tx_mod_am.
 *  \param audio_rate The sample rate of the audio input (and output).
 *  \param mod_index The modulation index; 1.0 is 100% modulation.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, tx_mod_am's constructor is private.
 * make_tx_mod_am is the public interface for creating new instances.
 */
tx_mod_am_sptr make_tx_mod_am(float audio_rate, float mod_index = 0.9);

/*! \brief Double-sideband AM modulator.
 *  \ingroup DSP
 *
 * The input is audio (float) at audio_rate; the output is the complex
 * baseband signal at the same rate. The flowgraph is:
 *
 *   audio -> band-pass -> limiter -> ( m * x + 1 ) -> I/Q
 *
 * The modulated signal is real-valued: the carrier (DC) is present in
 * the I component and the modulation index is limited to 1.0 so that
 * the envelope can not go negative (overmodulation).
 */
class tx_mod_am : public gr::hier_block2
{

public:
    tx_mod_am(float audio_rate, float mod_index); // FIXME: should be private
    ~tx_mod_am();

    void set_mod_index(float mod_index);

private:
    /* GR blocks */
    gr::filter::fft_filter_fff::sptr       d_bpf;    /*! Audio band-pass filter. */
    gr::analog::rail_ff::sptr              d_limit;  /*! Audio limiter. */
    gr::blocks::multiply_const_ff::sptr    d_scale;  /*! Scales audio to m * x. */
    gr::blocks::add_const_ff::sptr         d_carrier;/*! Adds the carrier (1.0). */
    gr::blocks::float_to_complex::sptr     d_f2c;    /*! Float to complex. */

    /* other parameters */
    float       d_audio_rate; /*! Audio sample rate. */
    float       d_mod_index;  /*! Modulation index. */
};
