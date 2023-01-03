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


nbrx_sptr make_nbrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new nbrx(quad_rate, audio_rate));
}

nbrx::nbrx(float quad_rate, float audio_rate)
    : receiver_base_cf("NBRX", NB_PREF_QUAD_RATE, quad_rate, audio_rate),
      d_running(false)
{

    nb = make_rx_nb_cc((double)NB_PREF_QUAD_RATE, 3.3, 2.5);
    filter = make_rx_filter((double)NB_PREF_QUAD_RATE, -5000.0, 5000.0, 1000.0);
    demod_raw = gr::blocks::complex_to_float::make(1);
    demod_ssb = gr::blocks::complex_to_real::make(1);
    demod_fm = make_rx_demod_fm(NB_PREF_QUAD_RATE, 5000.0, 75.0e-6);
    demod_am = make_rx_demod_am(NB_PREF_QUAD_RATE, true);
    demod_amsync = make_rx_demod_amsync(NB_PREF_QUAD_RATE, true, 0.001);

    // Width of rx_filter can be adjusted at run time, so the input buffer (the
    // output buffer of nb) needs to be large enough for the longest history
    // required by the filter (Narrow/Sharp). This setting may not be reliable
    // for GR prior to v3.10.7.0.
    nb->set_min_output_buffer(32768);

    audio_rr0.reset();
    audio_rr1.reset();
    if (d_audio_rate != NB_PREF_QUAD_RATE)
    {
        std::cout << "Resampling audio " << NB_PREF_QUAD_RATE << " -> "
                  << d_audio_rate << std::endl;
        audio_rr0 = make_resampler_ff(d_audio_rate/NB_PREF_QUAD_RATE);
        audio_rr1 = make_resampler_ff(d_audio_rate/NB_PREF_QUAD_RATE);
    }

    demod = demod_raw;
    connect(ddc, 0, iq_resamp, 0);
    connect(iq_resamp, 0, nb, 0);
    connect(nb, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(agc, 2, self(), 0);
    connect(agc, 3, self(), 1);
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

void nbrx::set_filter(int low, int high, int tw)
{
    receiver_base_cf::set_filter(low, high, tw);
    filter->set_param(double(low), double(high), double(tw));
}

void nbrx::set_cw_offset(int offset)
{
    if(offset==get_cw_offset())
        return;
    vfo_s::set_cw_offset(offset);
    switch (get_demod())
    {
    case Modulations::MODE_CWL:
        ddc->set_center_freq(get_offset() + get_cw_offset());
        filter->set_cw_offset(-get_cw_offset());
        break;
    case Modulations::MODE_CWU:
        ddc->set_center_freq(get_offset() - get_cw_offset());
        filter->set_cw_offset(get_cw_offset());
        break;
    default:
        ddc->set_center_freq(get_offset());
        filter->set_cw_offset(0);
    }
}

void nbrx::set_offset(int offset)
{
    if(offset==get_offset())
        return;
    vfo_s::set_offset(offset);
    switch (get_demod())
    {
    case Modulations::MODE_CWL:
        ddc->set_center_freq(offset + get_cw_offset());
        break;
    case Modulations::MODE_CWU:
        ddc->set_center_freq(offset - get_cw_offset());
        break;
    default:
        ddc->set_center_freq(offset);
    }
    wav_sink->set_offset(offset);
}

void nbrx::set_nb_on(int nbid, bool on)
{
    receiver_base_cf::set_nb_on(nbid, on);
    if (nbid == 1)
        nb->set_nb1_on(on);
    else if (nbid == 2)
        nb->set_nb2_on(on);
}

void nbrx::set_nb_threshold(int nbid, float threshold)
{
    receiver_base_cf::set_nb_threshold(nbid, threshold);
    if (nbid == 1)
        nb->set_threshold1(threshold);
    else if (nbid == 2)
        nb->set_threshold2(threshold);
}

void nbrx::set_demod(Modulations::idx new_demod)
{
    Modulations::idx current_demod = receiver_base_cf::get_demod();

    if (new_demod == current_demod) {
        /* nothing to do */
        return;
    }

    /* check if new demodulator selection is valid */
    if ((new_demod < Modulations::MODE_OFF) || (new_demod > Modulations::MODE_NFM))
        return;


    if (current_demod > Modulations::MODE_OFF)
    {
        disconnect(sql, 0, demod, 0);
        if (audio_rr0)
        {
            if (current_demod == Modulations::MODE_RAW)
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
            if (current_demod == Modulations::MODE_RAW)
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
    }

    switch (new_demod) {

    case Modulations::MODE_RAW:
    case Modulations::MODE_OFF:
        demod = demod_raw;
        break;

    case Modulations::MODE_LSB:
    case Modulations::MODE_USB:
    case Modulations::MODE_CWL:
    case Modulations::MODE_CWU:
        demod = demod_ssb;
        break;

    case Modulations::MODE_AM:
        demod = demod_am;
        break;

    case Modulations::MODE_AM_SYNC:
        demod = demod_amsync;
        break;

    case Modulations::MODE_NFM:
    default:
        demod = demod_fm;
        break;
    }

    if (new_demod > Modulations::MODE_OFF)
    {
        connect(sql, 0, demod, 0);
        if (audio_rr0)
        {
            if (new_demod == Modulations::MODE_RAW)
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
            if (new_demod == Modulations::MODE_RAW)
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
    receiver_base_cf::set_demod(new_demod);
    switch (get_demod())
    {
    case Modulations::MODE_CWL:
        ddc->set_center_freq(get_offset() + get_cw_offset());
        filter->set_cw_offset(-get_cw_offset());
        break;
    case Modulations::MODE_CWU:
        ddc->set_center_freq(get_offset() - get_cw_offset());
        filter->set_cw_offset(get_cw_offset());
        break;
    default:
        ddc->set_center_freq(get_offset());
        filter->set_cw_offset(0);
    }
}

void nbrx::set_fm_maxdev(float maxdev_hz)
{
    receiver_base_cf::set_fm_maxdev(maxdev_hz);
    demod_fm->set_max_dev(maxdev_hz);
}

void nbrx::set_fm_deemph(double tau)
{
    receiver_base_cf::set_fm_deemph(tau);
    demod_fm->set_tau(tau);
}

void nbrx::set_am_dcr(bool enabled)
{
    receiver_base_cf::set_am_dcr(enabled);
    if(get_demod() != Modulations::MODE_OFF)
        lock();
    demod_am->set_dcr(enabled);
    if(get_demod() != Modulations::MODE_OFF)
        unlock();
}

void nbrx::set_amsync_dcr(bool enabled)
{
    receiver_base_cf::set_amsync_dcr(enabled);
    if(get_demod() != Modulations::MODE_OFF)
        lock();
    demod_amsync->set_dcr(enabled);
    if(get_demod() != Modulations::MODE_OFF)
        unlock();
}

void nbrx::set_amsync_pll_bw(float pll_bw)
{
    receiver_base_cf::set_amsync_pll_bw(pll_bw);
    demod_amsync->set_pll_bw(pll_bw);
}
