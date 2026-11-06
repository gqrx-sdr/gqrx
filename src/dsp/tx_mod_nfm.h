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

#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/filter/rational_resampler.h>

#include "dsp/tx_modulator.h"

/* -----------------------------------------------------------------------
 * Narrowband FM TX modulator.
 *
 * Uses gr::analog::frequency_modulator_fc (GR 3.10 compatible).
 * gr::analog::nbfm_tx was removed in GR 3.10.
 *
 * Signal chain:
 *   audio (float, audio_rate Hz)
 *     → gr::analog::frequency_modulator_fc  (FM mod at audio_rate)
 *         sensitivity = 2π * max_dev / audio_rate
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
                                 float        max_dev  = 5000.f);

class tx_mod_nfm : public tx_modulator_base
{
public:
    static tx_mod_nfm_sptr make(unsigned int audio_rate,
                                double       rf_rate,
                                float        max_dev = 5000.f);

    /** Update FM sensitivity from new max deviation (Hz). */
    void set_max_dev(float dev_hz);

    float  max_dev() const { return d_max_dev; }

private:
    tx_mod_nfm(unsigned int audio_rate,
               double       rf_rate,
               float        max_dev);

    float  d_max_dev;

    gr::analog::frequency_modulator_fc::sptr       d_fm;
    gr::filter::rational_resampler_ccc::sptr        d_resamp;
};
