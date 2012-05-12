/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include <iostream>
#include "input/fcdctl/fcd.h"
#include "input/fcdctl/fcdhidcmd.h"
#include "rx_source_fcd.h"


rx_source_fcd_sptr make_rx_source_fcd(const std::string device_name)
{
    return gnuradio::get_initial_sptr(new rx_source_fcd(device_name));
}


rx_source_fcd::rx_source_fcd(const std::string device_name)
    : rx_source_base("rx_source_fcd"),
      d_freq(144.5e6),
      d_freq_corr(-12), // S/N 820 or greater
      d_gain(20.0)
{
    d_audio_src = make_pa_source(device_name, 96000, 2, "GQRX", "I/Q input");
    /** TODO: check error */

    d_f2c = gr_make_float_to_complex(1);

    // populate supported sample rates
    d_sample_rates.push_back(96000.0);

    connect(d_audio_src, 0, d_f2c, 0);
    connect(d_audio_src, 1, d_f2c, 1);
    connect(d_f2c, 0, self(), 0);

    set_freq(144.5e6f);
    set_gain(20.0f);
}


rx_source_fcd::~rx_source_fcd()
{

}


void rx_source_fcd::select_device(const std::string device_name)
{
    d_audio_src->select_device(device_name);

    /** FIXME **/
    //d_fcd_src->set_freq((float) d_freq);
    //d_fcd_src->set_lna_gain((float) d_gain);
}

void rx_source_fcd::set_freq(double freq)
{
    FCD_MODE_ENUM fme;
    double f = freq;

    if ((freq >= get_freq_min()) && (freq <= get_freq_max()))
    {
        d_freq = freq;

        f *= 1.0 + d_freq_corr/1000000.0;
        fme = fcdAppSetFreq((int)f);

        if (fme != FCD_MODE_APP)
        {
            /** FIXME: error message **/
        }
    }

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "FCD source new freq: " << (int) d_freq << " Hz" << std::endl;
#endif
}

double rx_source_fcd::get_freq()
{
    return d_freq;
}

double rx_source_fcd::get_freq_min()
{
    return 50.0e6;
}

double rx_source_fcd::get_freq_max()
{
    return 2.0e9;
}

void rx_source_fcd::set_gain(double gain)
{
    FCD_MODE_ENUM fme;
    unsigned char g;

    if ((gain >= get_gain_min()) && (gain <= get_gain_max()))
    {
        d_gain = gain;

        /* convert gain to nearest discrete value */
        if (gain > 27.5)
            g = 14;              // 30.0 dB
        else if (gain > 22.5)
            g = 13;              // 25.0 dB
        else if (gain > 18.75)
            g = 12;              // 20.0 dB
        else if (gain > 16.25)
            g = 11;              // 17.5 dB
        else if (gain > 13.75)
            g = 10;              // 15.0 dB
        else if (gain > 11.25)
            g = 9;               // 12.5 dB
        else if (gain > 8.75)
            g = 8;               // 10.0 dB
        else if (gain > 6.25)
            g = 7;               // 7.5 dB
        else if (gain > 3.75)
            g = 6;               // 5.0 dB
        else if (gain > 1.25)
            g = 5;               // 2.5 dB
        else if (gain > -1.25)
            g = 4;               // 0.0 dB
        else if (gain > -3.75)
            g = 1;               // -2.5 dB
        else
            g = 0;               // -5.0 dB

        fme = fcdAppSetParam(FCD_CMD_APP_SET_LNA_GAIN, &g, 1);
        if (fme != FCD_MODE_APP)
        {
            /** FIUXME: error message **/
        }
    }
}

double rx_source_fcd::get_gain()
{
    return d_gain;
}

double rx_source_fcd::get_gain_min()
{
    return -5.0;
}

double rx_source_fcd::get_gain_max()
{
    return 30.0;
}

void rx_source_fcd::set_sample_rate(double sps)
{
    // nothing to do;
}

double rx_source_fcd::get_sample_rate()
{
    return 96000.0;
}

std::vector<double> rx_source_fcd::get_sample_rates()
{
    return d_sample_rates;
}

/** FIXME: Remove? */
void rx_source_fcd::set_freq_corr(int ppm)
{
    d_freq_corr = ppm;
    set_freq(d_freq);
}

/** FIXME: Remove? */
void rx_source_fcd::set_dc_corr(double dci, double dcq)
{
    //d_fcd_src->set_dc_corr(dci, dcq);
}

/** FIXME: Remove? */
void rx_source_fcd::set_iq_corr(double _gain, double _phase)
{
    FCD_MODE_ENUM fme;
    union {
        unsigned char auc[4];
        struct {
            signed short phase;
            signed short gain;
        };
    } iqinfo;

    if ((_gain < -1.0) || (_gain > 1.0) || (_phase < -1.0) || (_phase > 1.0))
        return;

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "FCD source I/Q corr phase:" << _phase << " gain:" << _gain << std::endl;
#endif

    iqinfo.phase = static_cast<signed short>(_phase*32768.0);
    iqinfo.gain = static_cast<signed short>(_gain*32768.0);

    fme = fcdAppSetParam(FCD_CMD_APP_SET_IQ_CORR, iqinfo.auc, 4);
    if (fme != FCD_MODE_APP)
    {
        /** FIUXME: error message **/
    }
}
