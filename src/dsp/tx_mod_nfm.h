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

#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/blocks/add_ff.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/hier_block2.h>

class tx_mod_nfm;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_nfm> tx_mod_nfm_sptr;
#else
typedef std::shared_ptr<tx_mod_nfm> tx_mod_nfm_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of tx_mod_nfm.
 *  \param audio_rate The sample rate of the audio input (and output).
 *  \param max_dev Maximum deviation in Hz.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, tx_mod_nfm's constructor is private.
 * make_tx_mod_nfm is the public interface for creating new instances.
 */
tx_mod_nfm_sptr make_tx_mod_nfm(float audio_rate, float max_dev = 5000.0);

/*! \brief Narrow-band FM modulator.
 *  \ingroup DSP
 *
 * The input is audio (float) at audio_rate; the output is the complex
 * baseband signal at the same rate. The flowgraph is:
 *
 *   audio -> band-pass -> limiter -> ( + CTCSS tone ) -> freq modulator
 *
 * The band-pass filter limits the occupied bandwidth of the voice
 * signal and the limiter prevents overdriving the modulator, so the
 * maximum deviation can not exceed max_dev even with an overdriven
 * input.
 *
 * The frequency modulator gain is the exact inverse of the quadrature
 * demodulator gain used on the receiving side:
 *
 *   sens = 2 * PI * max_dev / rate
 */
class tx_mod_nfm : public gr::hier_block2
{

public:
    tx_mod_nfm(float audio_rate, float max_dev); // FIXME: should be private
    ~tx_mod_nfm();

    void set_max_dev(float max_dev);
    void set_ctcss(float freq);

private:
    /* GR blocks */
    gr::filter::fft_filter_fff::sptr       d_bpf;    /*! Audio band-pass filter. */
    gr::analog::rail_ff::sptr              d_limit;  /*! Audio limiter. */
    gr::analog::sig_source_f::sptr         d_ctcss;  /*! CTCSS tone generator. */
    gr::blocks::add_ff::sptr               d_adder;  /*! Sums voice + CTCSS. */
    gr::analog::frequency_modulator_fc::sptr d_mod;  /*! The frequency modulator block. */

    /* other parameters */
    float       d_audio_rate; /*! Audio sample rate. */
    float       d_max_dev;    /*! Maximum deviation. */
};
