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

#include <gnuradio/blocks/conjugate_cc.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/hilbert_fc.h>
#include <gnuradio/hier_block2.h>

class tx_mod_ssb;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_ssb> tx_mod_ssb_sptr;
#else
typedef std::shared_ptr<tx_mod_ssb> tx_mod_ssb_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of tx_mod_ssb.
 *  \param audio_rate The sample rate of the audio input (and output).
 *  \param upper Whether to generate USB (true) or LSB (false).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, tx_mod_ssb's constructor is private.
 * make_tx_mod_ssb is the public interface for creating new instances.
 */
tx_mod_ssb_sptr make_tx_mod_ssb(float audio_rate, bool upper = true);

/*! \brief Single sideband modulator (USB/LSB).
 *  \ingroup DSP
 *
 * The input is audio (float) at audio_rate; the output is the complex
 * baseband signal at the same rate. The flowgraph is:
 *
 *   audio -> band-pass -> Hilbert transform -> [ conj ] -> I/Q
 *
 * The Hilbert transform converts the real voice signal into its
 * analytic signal, which occupies only the positive frequencies.
 * Transmitting the analytic signal directly produces an upper
 * sideband signal; complex conjugation of it produces a lower
 * sideband signal. The carrier is suppressed, as expected for SSB.
 */
class tx_mod_ssb : public gr::hier_block2
{

public:
    tx_mod_ssb(float audio_rate, bool upper); // FIXME: should be private
    ~tx_mod_ssb();

    void set_upper(bool upper);

private:
    /* GR blocks */
    gr::filter::fft_filter_fff::sptr    d_bpf;   /*! Audio band-pass filter. */
    gr::filter::hilbert_fc::sptr        d_hilbert; /*! Hilbert transform. */
    gr::blocks::conjugate_cc::sptr      d_conj;  /*! Complex conjugate. */

    /* other parameters */
    float       d_audio_rate; /*! Audio sample rate. */
    bool        d_upper;      /*! Whether the sideband is upper or lower. */
};
