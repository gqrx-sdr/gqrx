/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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

#include <gnuradio/filter/hilbert_fc.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/rational_resampler.h>

#include "dsp/tx_modulator.h"

/* -----------------------------------------------------------------------
 * SSB (USB or LSB) TX modulator using the phasing method.
 *
 * USB signal chain:
 *   audio (float, audio_rate Hz)
 *     → gr::filter::hilbert_fc  (analytic signal — USB by definition)
 *     → rational_resampler_ccc  (upsample to rf_rate)
 *     → output (complex, rf_rate Hz)
 *
 * LSB: after hilbert_fc negate the imaginary part by multiplying by
 *      gr_complex(1, -1) which conjugates the signal, mirroring the
 *      spectrum around 0 Hz (USB ↔ LSB).
 *
 * Note: hilbert_fc in GNU Radio uses a 65-tap default FIR; the number
 *       of taps must be odd and ≥ 1.  Default is fine for audio rates.
 * ----------------------------------------------------------------------- */
class tx_mod_ssb;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_ssb> tx_mod_ssb_sptr;
#else
typedef std::shared_ptr<tx_mod_ssb>   tx_mod_ssb_sptr;
#endif

tx_mod_ssb_sptr make_tx_mod_ssb(unsigned int audio_rate,
                                 double       rf_rate,
                                 bool         is_usb = true);

class tx_mod_ssb : public tx_modulator_base
{
public:
    static tx_mod_ssb_sptr make(unsigned int audio_rate,
                                 double       rf_rate,
                                 bool         is_usb = true);

    bool is_usb() const { return d_is_usb; }

private:
    tx_mod_ssb(unsigned int audio_rate,
                double       rf_rate,
                bool         is_usb);

    bool d_is_usb;

    gr::filter::hilbert_fc::sptr              d_hilbert;
    gr::blocks::multiply_const_cc::sptr       d_conj;    /*!< LSB only: conjugate = multiply by (1,-1j) */
    gr::filter::rational_resampler_ccc::sptr  d_resamp;
};

