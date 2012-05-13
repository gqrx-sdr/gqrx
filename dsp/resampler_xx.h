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
#ifndef RESAMPLER_XX_H
#define RESAMPELR_XX_H

#include <gr_hier_block2.h>
#include <gr_pfb_arb_resampler_ccf.h>

class resampler_cc;

typedef boost::shared_ptr<resampler_cc> resampler_cc_sptr;


/*! \brief Return a shared_ptr to a new instance of resampler_cc.
 *  \param rate Resampling rate, i.e. output/input (tbc).
 *
 * This is effectively the public constructor.
 */
resampler_cc_sptr make_resampler_cc(float rate);


/*! \brief Rational resampler based on gr_pfb_arb_resampler_ccf
 *  \ingroup DSP
 *
 * This block is a convenience wrapper around gr_pfb_arb_resampler_ccf. It takes care
 * of generating filter taps that can be used for the resampler, as well as calculating
 * the other required parameters.
 */
class resampler_cc : public gr_hier_block2
{

public:
    resampler_cc(float rate); // FIXME: should be private
    ~resampler_cc();

    void set_rate(float rate) { d_filter->set_rate(rate); }

private:
    std::vector<float>            d_taps;
    gr_pfb_arb_resampler_ccf_sptr d_filter;
};


#endif // RESAMPLER_XX_H
