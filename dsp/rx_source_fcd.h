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
#ifndef RX_SOURCE_FCD_H
#define RX_SOURCE_FCD_H

#include <gr_hier_block2.h>
#include <fcd/fcd_source_c.h>
#include <dsp/rx_source_base.h>


class rx_source_fcd;

/*! \brief Boost shared pointer to rx_source_fcd. */
typedef boost::shared_ptr<rx_source_fcd> rx_source_fcd_sptr;

/*! \brief Public constructor of rx_source_fcd. */
rx_source_fcd_sptr make_rx_source_fcd(const std::string device_name = "hw:1");


/*! \brief Wrapper block for Funcube Dongle source.
 *  \ingroup DSP
 *
 * This block provides a wrapper for the FCD source using the
 * rx_source_base API.
 *
 */
class rx_source_fcd : public rx_source_base
{

public:
    rx_source_fcd(const std::string device_name);
    ~rx_source_fcd();

    void select_device(const std::string device_name);

    void set_freq(double freq);
    double get_freq();
    double get_freq_min();
    double get_freq_max();

    void set_gain(double gain);
    double get_gain();
    double get_gain_min();
    double get_gain_max();

    void set_sample_rate(double sps);
    double get_sample_rate();
    std::vector<double> get_sample_rates();

    void set_freq_corr(int ppm);
    void set_dc_corr(double dci, double dcq);
    void set_iq_corr(double gain, double phase);

private:
    fcd_source_c_sptr   d_fcd_src;   /*! Funcube Dongle source. */

    std::vector<double> d_sample_rates;
    double              d_freq;
    double              d_gain;
};

#endif // RX_SOURCE_FCD_H
