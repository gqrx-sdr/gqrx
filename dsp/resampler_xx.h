/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
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
#define RESAMPLER_XX_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/filter/pfb_arb_resampler_fff.h>


class resampler_cc;
class resampler_ff;

typedef boost::shared_ptr<resampler_cc> resampler_cc_sptr;
typedef boost::shared_ptr<resampler_ff> resampler_ff_sptr;


/*! \brief Return a shared_ptr to a new instance of resampler_cc.
 *  \param rate Resampling rate, i.e. output/input.
 *
 * This is effectively the public constructor.
 */
resampler_cc_sptr make_resampler_cc(float rate);

/*! \brief Arbitrary rate resampler based on gr_pfb_arb_resampler_ccf
 *  \ingroup DSP
 *
 * This block is a convenience wrapper around gr_pfb_arb_resampler_ccf. It takes care
 * of generating filter taps that can be used for the filter, as well as calculating
 * the other required parameters.
 */
class resampler_cc : public gr::hier_block2
{

public:
    resampler_cc(float rate); // FIXME: should be private
    ~resampler_cc();

    void set_rate(float rate);

private:
    std::vector<float>            d_taps;
    gr::filter::pfb_arb_resampler_ccf::sptr d_filter;
};


/*! \brief Return a shared_ptr to a new instance of resampler_ff.
 *  \param rate Resampling rate, i.e. output/input.
 *
 * This is effectively the public constructor.
 */
resampler_ff_sptr make_resampler_ff(float rate);


/*! \brief Arbitrary rate resampler based on gr_pfb_arb_resampler_fff
 *  \ingroup DSP
 *
 * This block is a convenience wrapper around gr_pfb_arb_resampler_fff. It takes care
 * of generating filter taps that can be used for the filter, as well as calculating
 * the other required parameters.
 */
class resampler_ff : public gr::hier_block2
{

public:
    resampler_ff(float rate); // FIXME: should be private
    ~resampler_ff();

    void set_rate(float rate);

private:
    std::vector<float>            d_taps;
    gr::filter::pfb_arb_resampler_fff::sptr d_filter;
};

#endif // RESAMPLER_XX_H
