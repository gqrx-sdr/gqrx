/* -*- c++ -*- */
/*
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#ifndef CORRECT_IQ_CC_H
#define CORRECT_IQ_CC_H

#include <gr_complex.h>
#include <gr_complex_to_xxx.h>
#include <gr_float_to_complex.h>
#include <gr_hier_block2.h>
#include <gr_single_pole_iir_filter_cc.h>
#include <gr_sub_cc.h>

class dc_corr_cc;
class iq_swap_cc;

typedef boost::shared_ptr<dc_corr_cc> dc_corr_cc_sptr;
typedef boost::shared_ptr<iq_swap_cc> iq_swap_cc_sptr;

/*! \brief Return a shared_ptr to a new instance of dc_corr_cc.
 *  \param sample_rate The sample rate
 *  \param tau The time constant for the filter
 */
dc_corr_cc_sptr make_dc_corr_cc(double sample_rate, double tau=0.0);

/*! \brief Single pole IIR filter-based DC offset correction block.
 *  \ingroup DSP
 *
 * This block performs automatic DC offset removal using a single pole IIR
 * filter
 */
class dc_corr_cc : public gr_hier_block2
{
    friend dc_corr_cc_sptr make_dc_corr_cc(double sample_rate, double tau);

protected:
    dc_corr_cc(double sample_rate, double tau);

public:
    ~dc_corr_cc();
    void set_sample_rate(double sample_rate);
    void set_tau(double tau);
    void set_enabled(bool enabled);

private:
    gr_single_pole_iir_filter_cc_sptr d_iir;
    gr_sub_cc_sptr                    d_sub;

    double d_period; /*!< Sampling period. */
    double d_tau;    /*!< Time constant. */
    double d_alpha;  /*!< 1/(1+tau/T). */
};



/*! \brief Return a shared_ptr to a new instance of iq_swap_cc. */
iq_swap_cc_sptr make_iq_swap_cc(bool enabled);

/*! \brief Block to swap I and Q channels.
 *  \ingroup DSP
 */
class iq_swap_cc : public gr_hier_block2
{
    friend iq_swap_cc_sptr make_iq_swap_cc(bool enabled);

protected:
    iq_swap_cc(bool enabled);

public:
    ~iq_swap_cc();
    void set_enabled(bool enabled);

private:
    gr_complex_to_float_sptr d_c2f;
    gr_float_to_complex_sptr d_f2c;
    bool d_enabled;
};




#endif /* CORRECT_IQ_CC_H */
