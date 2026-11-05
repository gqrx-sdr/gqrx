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
#include <cmath>
#include "dsp/tx_mod_nfm.h"

tx_mod_nfm_sptr make_tx_mod_nfm(unsigned int audio_rate,
                                 double       rf_rate,
                                 float        max_dev,
                                 double       tau)
{
    return tx_mod_nfm::make(audio_rate, rf_rate, max_dev, tau);
}

tx_mod_nfm_sptr tx_mod_nfm::make(unsigned int audio_rate,
                                  double       rf_rate,
                                  float        max_dev,
                                  double       tau)
{
    return tx_mod_nfm_sptr(new tx_mod_nfm(audio_rate, rf_rate, max_dev, tau));
}

tx_mod_nfm::tx_mod_nfm(unsigned int audio_rate,
                         double       rf_rate,
                         float        max_dev,
                         double       tau)
    : tx_modulator_base("tx_mod_nfm", audio_rate, rf_rate),
      d_max_dev(max_dev),
      d_tau(tau)
{
    /* nbfm_tx works at the audio rate and produces complex IQ */
    d_nbfm  = gr::analog::nbfm_tx::make(
                    static_cast<float>(audio_rate),  /* audio rate  */
                    static_cast<float>(audio_rate),  /* quad rate — same, no internal decimation */
                    max_dev,
                    tau);

    /* Upsample from audio_rate to rf_rate.
     * rational_resampler accepts integer interpolation/decimation.
     * We use gcd to reduce the fraction. */
    int    interp = static_cast<int>(rf_rate);
    int    decim  = static_cast<int>(audio_rate);
    int    g      = 1;
    /* Simple GCD via Euclidean algorithm */
    {
        int a = interp, b = decim;
        while (b) { int t = b; b = a % b; a = t; }
        g = a;
    }
    interp /= g;
    decim  /= g;

    d_resamp = gr::filter::rational_resampler_ccc::make(interp, decim);

    /* Connect: nbfm_tx → resampler */
    connect(self(), 0, d_nbfm,   0);
    connect(d_nbfm,  0, d_resamp, 0);
    connect(d_resamp, 0, self(),  0);
}

void tx_mod_nfm::set_max_dev(float dev_hz)
{
    d_max_dev = dev_hz;
    d_nbfm->set_max_deviation(dev_hz);
}

void tx_mod_nfm::set_tau(double tau)
{
    d_tau = tau;
    /* nbfm_tx does not expose set_tau post-construction; would need rebuild.
     * Store for future use / reconstruction. */
}

