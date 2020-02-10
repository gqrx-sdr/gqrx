/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2020 Clayton Smith VE3IRR.
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
#include <math.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>

#include "downconverter.h"

#define LPF_CUTOFF 120e3

downconverter_cc_sptr make_downconverter_cc(unsigned int decim, double center_freq, double samp_rate)
{
    return gnuradio::get_initial_sptr(new downconverter_cc(decim, center_freq, samp_rate));
}

downconverter_cc::downconverter_cc(unsigned int decim, double center_freq, double samp_rate)
    : gr::hier_block2("downconverter_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_decim(decim),
      d_center_freq(center_freq),
      d_samp_rate(samp_rate)
{
    if (d_decim > 1)
        fft_filt = gr::filter::fft_filter_ccc::make(d_decim, {1});
    rot = gr::blocks::rotator_cc::make(0.0);
    connect_all();
    update_proto_taps();
    update_fft_taps();
    update_phase_inc();
}

downconverter_cc::~downconverter_cc()
{

}

void downconverter_cc::set_decim_and_samp_rate(unsigned int decim, double samp_rate)
{
    d_samp_rate = samp_rate;
    if (decim != d_decim)
    {
        d_decim = decim;
        lock();
        disconnect_all();
        fft_filt.reset();
        if (d_decim > 1)
            fft_filt = gr::filter::fft_filter_ccc::make(d_decim, {1});
        connect_all();
        unlock();
    }
    update_proto_taps();
    update_fft_taps();
    update_phase_inc();
}

void downconverter_cc::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
    update_fft_taps();
    update_phase_inc();
}

void downconverter_cc::connect_all()
{
    if (d_decim > 1)
    {
        connect(self(), 0, fft_filt, 0);
        connect(fft_filt, 0, rot, 0);
    }
    else
    {
        connect(self(), 0, rot, 0);
    }
    connect(rot, 0, self(), 0);
}

void downconverter_cc::update_proto_taps()
{
    if (d_decim > 1)
    {
        double out_rate = d_samp_rate / d_decim;
        d_proto_taps = gr::filter::firdes::low_pass(1.0, d_samp_rate, LPF_CUTOFF, out_rate - 2*LPF_CUTOFF);
    }
}

void downconverter_cc::update_fft_taps()
{
    if (d_decim > 1)
    {
        std::vector<gr_complex> ctaps(d_proto_taps.size());
        float fwT0 = 2 * M_PI * d_center_freq / d_samp_rate;
        for (unsigned int i = 0; i < d_proto_taps.size(); i++) {
            ctaps[i] = d_proto_taps[i] * exp(gr_complex(0, i * fwT0));
        }
        fft_filt->set_taps(ctaps);
    }
}

void downconverter_cc::update_phase_inc()
{
    rot->set_phase_inc(-2.0 * M_PI * d_decim * d_center_freq / d_samp_rate);
}
