/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QDebug>
#include "receivers/nbrx.h"

// NB: Remember to adjust filter ranges in MainWindow
#define PREF_QUAD_RATE  96000.f

nbrx_sptr make_nbrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new nbrx(quad_rate, audio_rate));
}

nbrx::nbrx(float quad_rate, float audio_rate)
    : receiver_base_cf("NBRX", PREF_QUAD_RATE, quad_rate, audio_rate),
      d_running(false),
      d_demod(NBRX_DEMOD_FM)
{

    nb = make_rx_nb_cc((double)PREF_QUAD_RATE, 3.3, 2.5);
    filter = make_rx_filter((double)PREF_QUAD_RATE, -5000.0, 5000.0, 1000.0);
    demod_raw = gr::blocks::complex_to_float::make(1);
    demod_ssb = gr::blocks::complex_to_real::make(1);
    demod_fm = make_rx_demod_fm(PREF_QUAD_RATE, 5000.0, 75.0e-6);
    demod_am = make_rx_demod_am(PREF_QUAD_RATE, true);
    demod_amsync = make_rx_demod_amsync(PREF_QUAD_RATE, true, 0.001);

    // Width of rx_filter can be adjusted at run time, so the input buffer (the
    // output buffer of nb) needs to be large enough for the longest history
    // required by the filter (Narrow/Sharp). This setting may not be reliable
    // for GR prior to v3.10.7.0.
    nb->set_min_output_buffer(32768);

    audio_rr0.reset();
    audio_rr1.reset();
    if (d_audio_rate != PREF_QUAD_RATE)
    {
        std::cout << "Resampling audio " << PREF_QUAD_RATE << " -> "
                  << d_audio_rate << std::endl;
        audio_rr0 = make_resampler_ff(d_audio_rate/PREF_QUAD_RATE);
        audio_rr1 = make_resampler_ff(d_audio_rate/PREF_QUAD_RATE);
    }

    demod = demod_fm;
    connect(self(), 0, iq_resamp, 0);
    connect(iq_resamp, 0, nb, 0);
    connect(nb, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(sql, 0, demod, 0);
//    connect(sql, 0, agc, 0);
//    connect(agc, 0, demod, 0);

    if (audio_rr0)
    {
        connect(demod, 0, audio_rr0, 0);

        connect(audio_rr0, 0, agc, 0); // left  channel
        connect(audio_rr0, 0, agc, 1); // right channel
    }
    else
    {
        connect(demod, 0, agc, 0);
        connect(demod, 0, agc, 1);
    }
    connect(agc, 0, self(), 0);
    connect(agc, 1, self(), 1);
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

void nbrx::set_filter(double low, double high, double tw)
{
    filter->set_param(low, high, tw);
}

void nbrx::set_cw_offset(double offset)
{
    filter->set_cw_offset(offset);
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

    disconnect(sql, 0, demod, 0);
    if (audio_rr0)
    {
        if (current_demod == NBRX_DEMOD_NONE)
        {
            disconnect(demod, 0, audio_rr0, 0);
            disconnect(demod, 1, audio_rr1, 0);

            disconnect(audio_rr0, 0, agc, 0);
            disconnect(audio_rr1, 0, agc, 1);
        }
        else
        {
            disconnect(demod, 0, audio_rr0, 0);

            disconnect(audio_rr0, 0, agc, 0);
            disconnect(audio_rr0, 0, agc, 1);
        }
    }
    else
    {
        if (current_demod == NBRX_DEMOD_NONE)
        {
            disconnect(demod, 0, agc, 0);
            disconnect(demod, 1, agc, 1);
        }
        else
        {
            disconnect(demod, 0, agc, 0);
            disconnect(demod, 0, agc, 1);
        }
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

    case NBRX_DEMOD_AMSYNC:
        d_demod = NBRX_DEMOD_AMSYNC;
        demod = demod_amsync;
        break;

    case NBRX_DEMOD_FM:
    default:
        d_demod = NBRX_DEMOD_FM;
        demod = demod_fm;
        break;
    }

    connect(sql, 0, demod, 0);
    if (audio_rr0)
    {
        if (d_demod == NBRX_DEMOD_NONE)
        {
            connect(demod, 0, audio_rr0, 0);
            connect(demod, 1, audio_rr1, 0);

            connect(audio_rr0, 0, agc, 0);
            connect(audio_rr1, 0, agc, 1);
        }
        else
        {
            connect(demod, 0, audio_rr0, 0);

            connect(audio_rr0, 0, agc, 0);
            connect(audio_rr0, 0, agc, 1);
        }
    }
    else
    {
        if (d_demod == NBRX_DEMOD_NONE)
        {
            connect(demod, 0, agc, 0);
            connect(demod, 1, agc, 1);
        }
        else
        {
            connect(demod, 0, agc, 0);
            connect(demod, 0, agc, 1);
        }
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

void nbrx::set_amsync_dcr(bool enabled)
{
    demod_amsync->set_dcr(enabled);
}

void nbrx::set_amsync_pll_bw(float pll_bw)
{
    demod_amsync->set_pll_bw(pll_bw);
}
