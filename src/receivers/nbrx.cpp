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
    : receiver_base_cf("NBRX"),
      d_running(false),
      d_quad_rate(quad_rate),
      d_audio_rate(audio_rate),
      d_demod(NBRX_DEMOD_FM)
{
    iq_resamp = make_resampler_cc(PREF_QUAD_RATE/d_quad_rate);

    nb = make_rx_nb_cc((double)PREF_QUAD_RATE, 3.3, 2.5);
    filter = make_rx_filter((double)PREF_QUAD_RATE, -5000.0, 5000.0, 1000.0);
    agc = make_rx_agc_cc((double)PREF_QUAD_RATE, true, -100, 0, 0, 500, false);
    sql = gr::analog::simple_squelch_cc::make(-150.0, 0.001);
    meter = make_rx_meter_c((double)PREF_QUAD_RATE);
    demod_raw = gr::blocks::complex_to_float::make(1);
    demod_ssb = gr::blocks::complex_to_real::make(1);
    demod_fm = make_rx_demod_fm(PREF_QUAD_RATE, 5000.0, 75.0e-6);
    demod_am = make_rx_demod_am(PREF_QUAD_RATE, true);
    demod_amsync = make_rx_demod_amsync(PREF_QUAD_RATE, true, 0.001);
    fax_decoder = gr::fax::fax_demod::make(PREF_QUAD_RATE,-425,425,120,576);
    fax_decoder_enable = false;
    d_rtty = gr::rtty::rtty_demod::make(PREF_QUAD_RATE,
                                        -225,225,
                                        50,
                                        gr::rtty::rtty_demod::RTTY_MODE_5BITS_BAUDOT,
                                        gr::rtty::rtty_demod::RTTY_PARITY_NONE);
    d_rtty_enable = false;

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
    connect(sql, 0, agc, 0);
    connect(agc, 0, demod, 0);

    if (audio_rr0)
    {
        connect(demod, 0, audio_rr0, 0);

        connect(audio_rr0, 0, self(), 0); // left  channel
        connect(audio_rr0, 0, self(), 1); // right channel
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
    if (std::abs(d_quad_rate-quad_rate) > 0.5f)
    {
        qDebug() << "Changing NB_RX quad rate:"  << d_quad_rate << "->" << quad_rate;
        d_quad_rate = quad_rate;
        lock();
        iq_resamp->set_rate(PREF_QUAD_RATE/d_quad_rate);
        unlock();
    }
}

void nbrx::set_filter(double low, double high, double tw)
{
    filter->set_param(low, high, tw);
}

void nbrx::set_cw_offset(double offset)
{
    filter->set_cw_offset(offset);
}

float nbrx::get_signal_level()
{
    return meter->get_level_db();
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
    if (audio_rr0)
    {
        if (current_demod == NBRX_DEMOD_NONE)
        {
            disconnect(demod, 0, audio_rr0, 0);
            disconnect(demod, 1, audio_rr1, 0);

            disconnect(audio_rr0, 0, self(), 0);
            disconnect(audio_rr1, 0, self(), 1);
        }
        else
        {
            disconnect(demod, 0, audio_rr0, 0);

            disconnect(audio_rr0, 0, self(), 0);
            disconnect(audio_rr0, 0, self(), 1);
        }
    }
    else
    {
        if (current_demod == NBRX_DEMOD_NONE)
        {
            disconnect(demod, 0, self(), 0);
            disconnect(demod, 1, self(), 1);
        }
        else
        {
            disconnect(demod, 0, self(), 0);
            disconnect(demod, 0, self(), 1);
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

    connect(agc, 0, demod, 0);
    if (audio_rr0)
    {
        if (d_demod == NBRX_DEMOD_NONE)
        {
            connect(demod, 0, audio_rr0, 0);
            connect(demod, 1, audio_rr1, 0);

            connect(audio_rr0, 0, self(), 0);
            connect(audio_rr1, 0, self(), 1);
        }
        else
        {
            connect(demod, 0, audio_rr0, 0);

            connect(audio_rr0, 0, self(), 0);
            connect(audio_rr0, 0, self(), 1);
        }
    }
    else
    {
        if (d_demod == NBRX_DEMOD_NONE)
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

int nbrx::start_decoder(enum rx_decoder decoder_type) {
    if (!is_decoder_active(decoder_type)) {
        switch (decoder_type) {
            case RX_DECODER_FAX:
                lock();
                connect(agc, 0, fax_decoder, 0);
                unlock();
                fax_decoder_enable = true;
                return 0;
            case RX_DECODER_RTTY:
                lock();
                connect(agc, 0, d_rtty, 0);
                d_rtty_enable = true;
                unlock();
                return 0;
            default:
                break;
        }
    }

    return -1;
}

int nbrx::stop_decoder(enum rx_decoder decoder_type) {
    if (is_decoder_active(decoder_type)) {
        switch (decoder_type) {
            case RX_DECODER_FAX:
                lock();
                disconnect(agc, 0, fax_decoder, 0);
                unlock();
                fax_decoder_enable = false;
                return 0;
            case RX_DECODER_RTTY:
                lock();
                d_rtty_enable = false;
                disconnect(agc, 0, d_rtty, 0);
                unlock();
                return 0;
            default:
                break;
        }
    }

    return -1;
}

bool nbrx::is_decoder_active(enum rx_decoder decoder_type) {
    switch (decoder_type) {
        case RX_DECODER_ANY:
            return d_rtty_enable || fax_decoder_enable;
        case RX_DECODER_FAX:
            return fax_decoder_enable;
        case RX_DECODER_RTTY:
            return d_rtty_enable;
        default:
            break;
    }
    return false;
}

int nbrx::reset_decoder(enum rx_decoder decoder_type) {
    switch (decoder_type) {
        case RX_DECODER_ALL:
        case RX_DECODER_FAX:
            fax_decoder->reset();
            return 0;
        case RX_DECODER_RTTY:
            d_rtty->reset();
            return 0;
        default:
            break;
    }

    return -1;
}

int nbrx::set_decoder_param(enum rx_decoder decoder_type, std::string param, std::string val) {
    switch (decoder_type) {
        case RX_DECODER_FAX:
            if (param == "black_freq")
                fax_decoder->set_black_freq(std::stof(val));
            else if (param == "white_freq")
                fax_decoder->set_white_freq(std::stof(val));
            else if (param == "lpm")
                fax_decoder->set_lpm(std::stof(val));
            else if (param == "ioc")
                fax_decoder->set_ioc(std::stof(val));
            else if (param == "force") {
                if (val == "reset")
                    fax_decoder->set_decoder_state(0);
                else if (val == "sync")
                    fax_decoder->set_decoder_state(2);
                else if (val == "start")
                    fax_decoder->set_decoder_state(5);
                else return -1;
            }
            else return -1;
            return 0;
        case RX_DECODER_RTTY:
            if (param == "mark_freq")
                d_rtty->set_mark_freq(std::stof(val));
            else if (param == "space_freq")
                d_rtty->set_space_freq(std::stof(val));
            else if (param == "threshold")
                d_rtty->set_threshold(std::stof(val));
            else if (param == "bandwidth")
                d_rtty->set_bandwidth(std::stof(val));
            else if (param == "transwidth")
                d_rtty->set_transwidth(std::stof(val));
            else if (param == "filterlen")
                d_rtty->set_filterlen(std::stof(val));
            else if (param == "baud_rate")
                d_rtty->set_baud_rate(std::stof(val));
            else if (param == "mode")
                d_rtty->set_mode((gr::rtty::rtty_demod::rtty_mode)std::stoi(val));
            else if (param == "parity")
                d_rtty->set_parity((gr::rtty::rtty_demod::rtty_parity)std::stoi(val));
            else return -1;
            return 0;
        default:
            break;
    }

    return -1;
}

int nbrx::get_decoder_param(enum rx_decoder decoder_type, std::string param, std::string &val) {
    switch (decoder_type) {
        case RX_DECODER_FAX:
            if (param == "black_freq")
                val = std::to_string(fax_decoder->black_freq());
            else if (param == "white_freq")
                val = std::to_string(fax_decoder->white_freq());
            else if (param == "lpm")
                val = std::to_string(fax_decoder->lpm());
            else if (param == "ioc")
                val = std::to_string(fax_decoder->ioc());
            else if (param == "state")
                switch (fax_decoder->decoder_state()) {
                    case 0: val = "Reset"; break;
                    case 1: val = "Wait start"; break;
                    case 2: val = "Wait white"; break;
                    case 3: val = "Measure white"; break;
                    case 4: val = "Measure black"; break;
                    case 5: val = "Get lines"; break;
                    default : return -1;
                }
            else return -1;
            return 0;
        case RX_DECODER_RTTY:
            if (param == "mark_freq")
                val = std::to_string(d_rtty->mark_freq());
            else if (param == "space_freq")
                val = std::to_string(d_rtty->space_freq());
            else if (param == "threshold")
                val = std::to_string(d_rtty->threshold());
            else if (param == "bandwidth")
                val = std::to_string(d_rtty->bandwidth());
            else if (param == "transwidth")
                val = std::to_string(d_rtty->transwidth());
            else if (param == "filterlen")
                val = std::to_string(d_rtty->filterlen());
            else if (param == "baud_rate")
                val = std::to_string(d_rtty->baud_rate());
            else if (param == "mode")
                val = std::to_string(d_rtty->mode());
            else if (param == "parity")
                val = std::to_string(d_rtty->parity());
            else return -1;
            return 0;
        default:
            break;
    }

    return -1;
}

int nbrx::get_decoder_data(enum rx_decoder decoder_type,void* data, int& num) {
    if (is_decoder_active(decoder_type)) {
        switch (decoder_type) {
            case RX_DECODER_FAX:
                if (num==-1)
                    return fax_decoder->get_data((unsigned char*)NULL,(unsigned int*)&num);
                else
                    return fax_decoder->get_data((unsigned char*)data,(unsigned int*)&num);
                break;
            case RX_DECODER_RTTY:
                num = d_rtty->get_data(*(std::string*)data);
                if (num == -1)
                    return -1;
                return 0;
            default:
                break;
        }
    }

    return -1;
}
