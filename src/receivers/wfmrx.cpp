/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
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
#include "receivers/wfmrx.h"

#define PREF_QUAD_RATE   240e3 // Nominal channel spacing is 200 kHz
#define PREF_MIDLE_RATE  120e3 // Midle rate for stereo decoder

wfmrx_sptr make_wfmrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new wfmrx(quad_rate, audio_rate));
}

wfmrx::wfmrx(float quad_rate, float audio_rate)
    : receiver_base_cf("WFMRX"),
      d_running(false),
      d_quad_rate(quad_rate),
      d_audio_rate(audio_rate),
      d_demod(WFMRX_DEMOD_MONO)
{
    iq_resamp = make_resampler_cc(PREF_QUAD_RATE/d_quad_rate);

    filter = make_rx_filter(PREF_QUAD_RATE, -80000.0, 80000.0, 20000.0);
    sql = gr::analog::simple_squelch_cc::make(-150.0, 0.001);
    meter = make_rx_meter_c(DETECTOR_TYPE_RMS);
    demod_fm = make_rx_demod_fm(PREF_QUAD_RATE, 75000.0, 50.0e-6);
    midle_rr = make_resampler_ff(PREF_MIDLE_RATE/PREF_QUAD_RATE);
    stereo = make_stereo_demod(PREF_MIDLE_RATE, d_audio_rate, true);
    stereo_oirt = make_stereo_demod(PREF_MIDLE_RATE, d_audio_rate, true, true);
    mono   = make_stereo_demod(PREF_MIDLE_RATE, d_audio_rate, false);

    /* create rds blocks but dont connect them */
    rds = make_rx_rds(PREF_QUAD_RATE);
    rds_decoder = gr::rds::decoder::make(0, 0);
    rds_parser = gr::rds::parser::make(0, 0);
    rds_store = make_rx_rds_store();
    rds_enabled = false;

    connect(self(), 0, iq_resamp, 0);
    connect(iq_resamp, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(sql, 0, demod_fm, 0);
    connect(demod_fm, 0, midle_rr, 0);
    connect(midle_rr, 0, mono, 0);
    connect(mono, 0, self(), 0); // left  channel
    connect(mono, 1, self(), 1); // right channel
}

wfmrx::~wfmrx()
{

}

bool wfmrx::start()
{
    d_running = true;

    return true;
}

bool wfmrx::stop()
{
    d_running = false;

    return true;
}

void wfmrx::set_quad_rate(float quad_rate)
{
    if (std::abs(d_quad_rate-quad_rate) > 0.5)
    {
#ifndef QT_NO_DEBUG_OUTPUT
        std::cerr << "Changing WFM RX quad rate: "  << d_quad_rate << " -> " << quad_rate << std::endl;
#endif
        d_quad_rate = quad_rate;
        lock();
        iq_resamp->set_rate(PREF_QUAD_RATE/d_quad_rate);
        unlock();
    }
}

void wfmrx::set_audio_rate(float audio_rate)
{
    (void) audio_rate;
}

void wfmrx::set_filter(double low, double high, double tw)
{
    filter->set_param(low, high, tw);
}

float wfmrx::get_signal_level(bool dbfs)
{
    if (dbfs)
        return meter->get_level_db();
    else
        return meter->get_level();

}

/*
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
*/

void wfmrx::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
}

void wfmrx::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
}

/*
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
*/

void wfmrx::set_demod(int demod)
{
    /* check if new demodulator selection is valid */
    if ((demod < WFMRX_DEMOD_MONO) || (demod >= WFMRX_DEMOD_NUM))
        return;

    if (demod == d_demod) {
        /* nothing to do */
        return;
    }

    /* lock graph while we reconfigure */
    lock();

    /* disconnect current demodulator */
    switch (d_demod) {

    case WFMRX_DEMOD_MONO:
    default:
        disconnect(midle_rr, 0, mono, 0);
        disconnect(mono, 0, self(), 0); // left  channel
        disconnect(mono, 1, self(), 1); // right channel
        break;

    case WFMRX_DEMOD_STEREO:
        disconnect(midle_rr, 0, stereo, 0);
        disconnect(stereo, 0, self(), 0); // left  channel
        disconnect(stereo, 1, self(), 1); // right channel
        break;

    case WFMRX_DEMOD_STEREO_UKW:
        disconnect(midle_rr, 0, stereo_oirt, 0);
        disconnect(stereo_oirt, 0, self(), 0); // left  channel
        disconnect(stereo_oirt, 1, self(), 1); // right channel
        break;
    }

    switch (demod) {

    case WFMRX_DEMOD_MONO:
    default:
        connect(midle_rr, 0, mono, 0);
        connect(mono, 0, self(), 0); // left  channel
        connect(mono, 1, self(), 1); // right channel
        break;

    case WFMRX_DEMOD_STEREO:
        connect(midle_rr, 0, stereo, 0);
        connect(stereo, 0, self(), 0); // left  channel
        connect(stereo, 1, self(), 1); // right channel
        break;

    case WFMRX_DEMOD_STEREO_UKW:
        connect(midle_rr, 0, stereo_oirt, 0);
        connect(stereo_oirt, 0, self(), 0); // left  channel
        connect(stereo_oirt, 1, self(), 1); // right channel
        break;
    }
    d_demod = (wfmrx_demod) demod;

    /* continue processing */
    unlock();
}

void wfmrx::set_fm_maxdev(float maxdev_hz)
{
    demod_fm->set_max_dev(maxdev_hz);
}

void wfmrx::set_fm_deemph(double tau)
{
    demod_fm->set_tau(tau);
}

void wfmrx::get_rds_data(std::string &outbuff, int &num)
{
    rds_store->get_message(outbuff, num);
}

void wfmrx::start_rds_decoder()
{
    connect(demod_fm, 0, rds, 0);
    connect(rds, 0, rds_decoder, 0);
    msg_connect(rds_decoder, "out", rds_parser, "in");
    msg_connect(rds_parser, "out", rds_store, "store");
    rds_enabled=true;
}

void wfmrx::stop_rds_decoder()
{
    lock();
    disconnect(demod_fm, 0, rds, 0);
    disconnect(rds, 0, rds_decoder, 0);
    msg_disconnect(rds_decoder, "out", rds_parser, "in");
    msg_disconnect(rds_parser, "out", rds_store, "store");
    unlock();
    rds_enabled=false;
}

void wfmrx::reset_rds_parser()
{
    rds_parser->reset();
}

bool wfmrx::is_rds_decoder_active()
{
    return rds_enabled;
}
