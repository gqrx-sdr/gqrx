/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#include <iostream>
#include "receivers/nbrx.h"

// NB: Remeber to adjust filter ranges in MainWindow
#define PREF_QUAD_RATE  96000.f

nbrx_sptr make_nbrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new nbrx(quad_rate, audio_rate));
}

nbrx::nbrx(float quad_rate, float audio_rate)
    : receiver_base_cf("NBRX"),
      d_running(false),
      d_quad_rate(quad_rate),
      d_audio_rate(audio_rate),
      d_demod(NBRX_DEMOD_FM)
{
    iq_resamp = make_resampler_cc(PREF_QUAD_RATE/d_quad_rate);

    nb = make_rx_nb_cc(PREF_QUAD_RATE, 3.3, 2.5);
    filter = make_rx_filter(PREF_QUAD_RATE, -5000.0, 5000.0, 1000.0);
    agc = make_rx_agc_cc(PREF_QUAD_RATE, true, -100, 0, 0, 500, false);
    sql = gr::analog::simple_squelch_cc::make(-150.0, 0.001);
    meter = make_rx_meter_c(DETECTOR_TYPE_RMS);
    demod_raw = gr::blocks::complex_to_float::make(1);
    demod_ssb = gr::blocks::complex_to_real::make(1);
    demod_fm = make_rx_demod_fm(PREF_QUAD_RATE, 5000.0, 75.0e-6);
    demod_am = make_rx_demod_am(PREF_QUAD_RATE, true);

    audio_rr.reset();
    if (d_audio_rate != PREF_QUAD_RATE)
    {
        std::cout << "Resampling audio " << PREF_QUAD_RATE << " -> "
                  << d_audio_rate << std::endl;
        audio_rr = make_resampler_ff(d_audio_rate/PREF_QUAD_RATE);
    }

    demod = demod_fm;
    connect(self(), 0, iq_resamp, 0);
    connect(iq_resamp, 0, nb, 0);
    connect(nb, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(sql, 0, agc, 0);
    connect(agc, 0, demod, 0);

    if (audio_rr)
    {
        connect(demod, 0, audio_rr, 0);
        connect(audio_rr, 0, self(), 0); // left  channel
        connect(audio_rr, 0, self(), 1); // right channel
    }
    else
    {
        connect(demod, 0, self(), 0);
        connect(demod, 0, self(), 1);
    }

}

bool nbrx::start()
{
    d_running = true;

    return true;
}

bool nbrx::stop()
{
    d_running = false;

    return true;
}

void nbrx::set_quad_rate(float quad_rate)
{
    if (std::abs(d_quad_rate-quad_rate) > 0.5)
    {
#ifndef QT_NO_DEBUG_OUTPUT
        std::cout << "Changing NB_RX quad rate: "  << d_quad_rate << " -> " << quad_rate << std::endl;
#endif
        d_quad_rate = quad_rate;
        lock();
        iq_resamp->set_rate(PREF_QUAD_RATE/d_quad_rate);
        unlock();
    }
}

void nbrx::set_audio_rate(float audio_rate)
{
    (void) audio_rate;
    std::cout << "**** FIXME: nbrx::set_audio_rate() not implemented" << std::endl;
}

void nbrx::set_filter(double low, double high, double tw)
{
    filter->set_param(low, high, tw);
}

void nbrx::set_cw_offset(double offset)
{
    filter->set_cw_offset(offset);
}

float nbrx::get_signal_level(bool dbfs)
{
    if (dbfs)
        return meter->get_level_db();
    else
        return meter->get_level();

}

void nbrx::set_nb_on(int nbid, bool on)
{
    if (nbid == 1)
        nb->set_nb1_on(on);
    else if (nbid == 2)
        nb->set_nb2_on(on);
}

void nbrx::set_nb_threshold(int nbid, float threshold)
{
    if (nbid == 1)
        nb->set_threshold1(threshold);
    else if (nbid == 2)
        nb->set_threshold2(threshold);
}

void nbrx::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
}

void nbrx::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
}

void nbrx::set_agc_on(bool agc_on)
{
    agc->set_agc_on(agc_on);
}

void nbrx::set_agc_hang(bool use_hang)
{
    agc->set_use_hang(use_hang);
}

void nbrx::set_agc_threshold(int threshold)
{
    agc->set_threshold(threshold);
}

void nbrx::set_agc_slope(int slope)
{
    agc->set_slope(slope);
}

void nbrx::set_agc_decay(int decay_ms)
{
    agc->set_decay(decay_ms);
}

void nbrx::set_agc_manual_gain(int gain)
{
    agc->set_manual_gain(gain);
}

void nbrx::set_demod(int rx_demod)
{
    nbrx_demod current_demod = d_demod;

    /* check if new demodulator selection is valid */
    if ((rx_demod < NBRX_DEMOD_NONE) || (rx_demod >= NBRX_DEMOD_NUM))
        return;

    if (rx_demod == current_demod) {
        /* nothing to do */
        return;
    }

    disconnect(agc, 0, demod, 0);
    if (audio_rr)
        disconnect(demod, 0, audio_rr, 0);
    else
    {
        disconnect(demod, 0, self(), 0);
        disconnect(demod, 0, self(), 1);
    }

    switch (rx_demod) {

    case NBRX_DEMOD_NONE:
        d_demod = NBRX_DEMOD_NONE;
        demod = demod_raw;
        break;

    case NBRX_DEMOD_SSB:
        d_demod = NBRX_DEMOD_SSB;
        demod = demod_ssb;
        break;

    case NBRX_DEMOD_AM:
        d_demod = NBRX_DEMOD_AM;
        demod = demod_am;
        break;

    case NBRX_DEMOD_FM:
    default:
        d_demod = NBRX_DEMOD_FM;
        demod = demod_fm;
        break;
    }

    connect(agc, 0, demod, 0);
    if (audio_rr)
    {
        // FIXME: DEMOD_NONE has two outputs.
        connect(demod, 0, audio_rr, 0);
    }
    else if (d_demod == NBRX_DEMOD_NONE)
    {
        connect(demod, 0, self(), 0);
        connect(demod, 1, self(), 1);
    }
    else
    {
        connect(demod, 0, self(), 0);
        connect(demod, 0, self(), 1);
    }

}

void nbrx::set_fm_maxdev(float maxdev_hz)
{
    demod_fm->set_max_dev(maxdev_hz);
}

void nbrx::set_fm_deemph(double tau)
{
    demod_fm->set_tau(tau);
}

void nbrx::set_am_dcr(bool enabled)
{
    demod_am->set_dcr(enabled);
}
