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

#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/filter/rational_resampler.h>

#include "dsp/tx_modulator.h"

/* -----------------------------------------------------------------------
 * AM (DSB full-carrier) TX modulator.
 *
 * Signal chain:
 *   audio (float, audio_rate Hz)
 *     → multiply_const_ff (mic_level, normalises to ±mod_index)
 *     → add_const_ff(1.0)          (adds carrier: s(t) = 1 + m·x(t))
 *     → float_to_complex           (real AM → complex, imaginary = 0)
 *     → rational_resampler_ccc     (upsample to rf_rate)
 *     → output (complex, rf_rate Hz)
 *
 * The carrier is at DC (0 Hz); the hardware sink shifts it to rf_freq.
 * ----------------------------------------------------------------------- */
class tx_mod_am;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_am> tx_mod_am_sptr;
#else
typedef std::shared_ptr<tx_mod_am>   tx_mod_am_sptr;
#endif

tx_mod_am_sptr make_tx_mod_am(unsigned int audio_rate,
                               double       rf_rate,
                               float        mod_index = 0.9f);

class tx_mod_am : public tx_modulator_base
{
public:
    static tx_mod_am_sptr make(unsigned int audio_rate,
                                double       rf_rate,
                                float        mod_index = 0.9f);

    /** Set modulation index (0.0–1.0). */
    void set_mod_index(float index);
    float mod_index() const { return d_mod_index; }

private:
    tx_mod_am(unsigned int audio_rate,
               double       rf_rate,
               float        mod_index);

    float d_mod_index;

    gr::blocks::multiply_const_ff::sptr  d_level;    /*!< Scale audio to ±mod_index */
    gr::blocks::add_const_ff::sptr       d_carrier;  /*!< Add DC carrier (1.0) */
    gr::blocks::float_to_complex::sptr   d_f2c;      /*!< Real → complex */
    gr::filter::rational_resampler_ccc::sptr d_resamp;
};

