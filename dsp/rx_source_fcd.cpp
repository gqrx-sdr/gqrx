/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
//#include <gr_io_signature.h>
#include <dsp/rx_source_fcd.h>


rx_source_fcd_sptr make_rx_source_fcd(const std::string device_name)
{
    return gnuradio::get_initial_sptr(new rx_source_fcd(device_name));
}



rx_source_fcd::rx_source_fcd(const std::string device_name)
    : rx_source_base("rx_source_fcd"),
      d_freq(144.5e6),
      d_gain(20.0)
{
    d_fcd_src = fcd_make_source_c(device_name);
    d_fcd_src->set_freq(144.5e6f);
    d_fcd_src->set_lna_gain(20.0f);

    /** TODO: check error */

    // populate supported sample rates
    d_sample_rates.push_back(96000.0);

    connect(d_fcd_src, 0, self(), 0);
}


rx_source_fcd::~rx_source_fcd()
{

}


void rx_source_fcd::select_device(const std::string device_name)
{
    // The only way to do this for now is to recreate FCD source
    lock();
    disconnect(d_fcd_src, 0, self(), 0);
    d_fcd_src.reset();
    d_fcd_src = fcd_make_source_c(device_name);
    d_fcd_src->set_freq((float) d_freq);
    d_fcd_src->set_lna_gain((float) d_gain);
    connect(d_fcd_src, 0, self(), 0);
    unlock();
}

void rx_source_fcd::set_freq(double freq)
{
    if ((freq >= get_freq_min()) && (freq <= get_freq_max())) {
        d_freq = freq;
        d_fcd_src->set_freq((float) d_freq);
    }
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
    if ((gain >= get_gain_min()) && (gain <= get_gain_max())) {
        d_gain = gain;
        d_fcd_src->set_lna_gain((float)gain);
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

void rx_source_fcd::set_freq_corr(int ppm)
{
    d_fcd_src->set_freq_corr(ppm);
    // re-tune after frequency correction
    d_fcd_src->set_freq((float) d_freq);
}

void rx_source_fcd::set_dc_corr(double dci, double dcq)
{
    d_fcd_src->set_dc_corr(dci, dcq);
}

void rx_source_fcd::set_iq_corr(double gain, double phase)
{
    d_fcd_src->set_iq_corr(gain, phase);
}
