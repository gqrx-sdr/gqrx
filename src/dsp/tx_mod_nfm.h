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

#include <gnuradio/analog/nbfm_tx.h>
#include <gnuradio/filter/rational_resampler.h>

#include "dsp/tx_modulator.h"

/* -----------------------------------------------------------------------
 * Narrowband FM TX modulator.
 *
 * Signal chain:
 *   audio (float, audio_rate Hz)
 *     → gr::analog::nbfm_tx   (FM modulation, produces complex at audio_rate)
 *     → gr::filter::rational_resampler_ccc  (upsample to rf_rate)
 *     → output (complex, rf_rate Hz)
 * ----------------------------------------------------------------------- */
class tx_mod_nfm;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_nfm> tx_mod_nfm_sptr;
#else
typedef std::shared_ptr<tx_mod_nfm>   tx_mod_nfm_sptr;
#endif

tx_mod_nfm_sptr make_tx_mod_nfm(unsigned int audio_rate,
                                 double       rf_rate,
                                 float        max_dev  = 5000.f,
                                 double       tau      = 75.0e-6);

class tx_mod_nfm : public tx_modulator_base
{
public:
    static tx_mod_nfm_sptr make(unsigned int audio_rate,
                                double       rf_rate,
                                float        max_dev = 5000.f,
                                double       tau     = 75.0e-6);

    /** Set maximum frequency deviation in Hz (live, via nbfm_tx). */
    void set_max_dev(float dev_hz);

    /** Set de-emphasis time constant in seconds. */
    void set_tau(double tau);

    float  max_dev() const { return d_max_dev; }
    double tau()     const { return d_tau; }

private:
    tx_mod_nfm(unsigned int audio_rate,
               double       rf_rate,
               float        max_dev,
               double       tau);

    float  d_max_dev;
    double d_tau;

    gr::analog::nbfm_tx::sptr                     d_nbfm;
    gr::filter::rational_resampler_ccc::sptr       d_resamp;
};

