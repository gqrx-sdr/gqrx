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
#ifndef RESAMPLER_FF_H
#define RESAMPELR_FF_H

#include <gr_hier_block2.h>
#include <gr_rational_resampler_base_fff.h>


class resampler_ffo;


typedef boost::shared_ptr<resampler_ffo> resampler_ffo_sptr;


/*! \brief Return a shared_ptr to a new instance of resampler_ff.
 *  \param input_rate Input sample rate in Hz.
 *  \param output_rate Output sample rate Hz.
 *
 * This is effectively the public constructor.
 */
resampler_ffo_sptr make_resampler_ffo(unsigned int input_rate, unsigned int output_rate);


/*! \brief Rational resampler based on gr_rational_resampler_base_fff
 *  \ingroup DSP
 *
 * This block is a convenience wrapper around gr_rational_resampler_fff. It takes care
 * of generating filter taps that can be used for the resampler, as well as calculating
 * the interpolation and decimation given the input/output sample rates.
 */
class resampler_ffo : public gr_hier_block2
{

public:
    resampler_ffo(unsigned int input_rate, unsigned int output_rate); // FIXME: should be private
    ~resampler_ffo();

private:
    std::vector<float> d_taps;
    gr_rational_resampler_base_fff_sptr  d_rrb;

    unsigned int d_interp;
    unsigned int d_decim;

    unsigned long long gcd(unsigned long long a, unsigned long long b);
    unsigned long long lcm(unsigned long long a, unsigned long long b);

};


#endif // RESAMPLER_FF_H
