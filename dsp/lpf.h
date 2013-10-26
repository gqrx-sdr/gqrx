/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#ifndef LPF_H
#define LPF_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_fff.h>


class lpf_ff;

typedef boost::shared_ptr<lpf_ff> lpf_ff_sptr;


/*! \brief Return a shared_ptr to a new instance of lpf.
 *  \param sample_rate The sample rate (Hz).
 *  \param cutoff_freq Center of transition band (Hz).
 *  \param transition_width Width of transition band (Hz).
 *  \param .
 *  \param gain Overall gain of filter (typically 1.0).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, lpf's constructor is private.
 * make_lpf is the public interface for creating new instances.
 */
lpf_ff_sptr make_lpf_ff(double sample_rate=48000.,
                        double cutoff_freq=5000.,
                        double trans_width=1000.,
                        double gain=1.0);

/*! \brief low-pass filter (LPF) with float taps.
 *  \ingroup DSP
 *
 * This class encapsulates a low-pass FIR filter and the code
 * required to generate filter taps. It provides a simple
 * interface to set the filter parameters.
 *
 * The user of this class is expected to provide valid parameters and no checks are
 * performed by the accessors (though the taps generator from gr::filter::firdes does perform
 * some sanity checks and throws std::out_of_range in case of bad parameter).
 */
class lpf_ff : public gr::hier_block2
{
  friend lpf_ff_sptr make_lpf_ff(double sample_rate, double cutoff_freq,
                                 double trans_width, double gain);
protected:
    lpf_ff(double sample_rate, double cutoff_freq,
           double trans_width, double gain);

public:
    ~lpf_ff();

    void set_param(double cutoff_freq, double trans_width);

private:
    /* GR blocks */
    gr::filter::fir_filter_fff::sptr lpf;

    /* other parameters */
    std::vector<float> d_taps;
    double d_sample_rate;
    double d_cutoff_freq;
    double d_trans_width;
    double d_gain;
};


#endif // LPF_H
