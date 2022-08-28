/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QDebug>
#include "receivers/wfmrx.h"

wfmrx_sptr make_wfmrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new wfmrx(quad_rate, audio_rate));
}

wfmrx::wfmrx(float quad_rate, float audio_rate)
    : receiver_base_cf("WFMRX", WFM_PREF_QUAD_RATE, quad_rate, audio_rate),
      d_running(false)
{

    filter = make_rx_filter((double)WFM_PREF_QUAD_RATE, -80000.0, 80000.0, 20000.0);
    demod_fm = make_rx_demod_fm(WFM_PREF_QUAD_RATE, 75000.0, 0.0);
    stereo = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true);
    stereo_oirt = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true, true);
    mono = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, false);

    /* create rds blocks but dont connect them */
    rds = make_rx_rds((double)WFM_PREF_QUAD_RATE);
    rds_decoder = gr::rds::decoder::make(0, 0);
    rds_parser = gr::rds::parser::make(0, 0, 0);
    rds_store = make_rx_rds_store();
    rds_enabled = false;

    connect(ddc, 0, iq_resamp, 0);
    connect(iq_resamp, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(sql, 0, demod_fm, 0);
    connect(demod_fm, 0, mono, 0);
    connect(mono, 0, agc, 0); // left  channel
    connect(mono, 1, agc, 1); // right channel
    connect(agc, 2, self(), 0);
    connect(agc, 3, self(), 1);
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

void wfmrx::set_filter(int low, int high, int tw)
{
    receiver_base_cf::set_filter(low, high, tw);
    filter->set_param(double(low), double(high), double(tw));
}

void wfmrx::set_demod(Modulations::idx demod)
{
    /* check if new demodulator selection is valid */
    if ((demod < Modulations::MODE_WFM_MONO) || (demod > Modulations::MODE_WFM_STEREO_OIRT))
        return;

    if (demod == receiver_base_cf::get_demod()) {
        /* nothing to do */
        return;
    }

    /* lock graph while we reconfigure */
    lock();

    /* disconnect current demodulator */
    switch (receiver_base_cf::get_demod()) {

    case Modulations::MODE_WFM_MONO:
    default:
        disconnect(demod_fm, 0, mono, 0);
        disconnect(mono, 0, agc, 0); // left  channel
        disconnect(mono, 1, agc, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO:
        disconnect(demod_fm, 0, stereo, 0);
        disconnect(stereo, 0, agc, 0); // left  channel
        disconnect(stereo, 1, agc, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        disconnect(demod_fm, 0, stereo_oirt, 0);
        disconnect(stereo_oirt, 0, agc, 0); // left  channel
        disconnect(stereo_oirt, 1, agc, 1); // right channel
        break;
    }

    switch (demod) {

    case Modulations::MODE_WFM_MONO:
    default:
        connect(demod_fm, 0, mono, 0);
        connect(mono, 0, agc, 0); // left  channel
        connect(mono, 1, agc, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO:
        connect(demod_fm, 0, stereo, 0);
        connect(stereo, 0, agc, 0); // left  channel
        connect(stereo, 1, agc, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        connect(demod_fm, 0, stereo_oirt, 0);
        connect(stereo_oirt, 0, agc, 0); // left  channel
        connect(stereo_oirt, 1, agc, 1); // right channel
        break;
    }
    receiver_base_cf::set_demod(demod);

    /* continue processing */
    unlock();
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
