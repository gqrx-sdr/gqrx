/* -*- c++ -*- */
/*
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
#include <math.h>
#include <gr_io_signature.h>
#include <gr_complex.h>
#include <gruel/high_res_timer.h>
#include <iostream>
#include "dsp/correct_iq_cc.h"


dc_corr_cc_sptr make_dc_corr_cc(float alpha)
{
    return gnuradio::get_initial_sptr(new dc_corr_cc(alpha));
}


/*! \brief Create DC correction object.
 *
 * Use make_dc_corr_cc() instead.
 */
dc_corr_cc::dc_corr_cc(float alpha)
    : gr_sync_block ("dc_corr_cc",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(1, 1, sizeof(gr_complex))),
      d_alpha(alpha),
      d_avg_i(0.0),
      d_avg_q(0.0)
{

}

dc_corr_cc::~dc_corr_cc()
{

}


/*! \brief DC correction block work method.
 *  \param moutput_items
 *  \param input_items
 *  \param output_items
 */
int dc_corr_cc::work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];
    int i;

    float sum_i = 0.0;
    float sum_q = 0.0;


    for (i = 0; i < noutput_items; i++)
    {
        sum_q += in[i].imag();
        sum_i += in[i].real();
    }

    d_avg_i = d_avg_i*(1.f-d_alpha) + d_alpha*(sum_i/noutput_items);
    d_avg_q = d_avg_q*(1.f-d_alpha) + d_alpha*(sum_q/noutput_items);

#ifndef QT_NO_DEBUG_OUTPUT
    gruel::high_res_timer_type tnow = gruel::high_res_timer_now();
    if ((tnow-d_dbg_timer) / gruel::high_res_timer_tps() > 5)
    {
        d_dbg_timer = tnow;
        std::cout << "AVG I/Q: " << d_avg_i << " / " << d_avg_q << std::endl;
    }
#endif

    for (i = 0; i < noutput_items; i++)
    {
        out[i].imag() = in[i].imag() - d_avg_q;
        out[i].real() = in[i].real() - d_avg_i;
    }

    return noutput_items;
}


bool dc_corr_cc::start()
{
    d_dbg_timer = gruel::high_res_timer_now();

    return true;
}

bool dc_corr_cc::stop()
{
    return true;
}
