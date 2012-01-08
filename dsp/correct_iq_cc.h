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
#ifndef CORRECT_IQ_CC_H
#define CORRECT_IQ_CC_H

#include <gr_sync_block.h>
#include <gr_complex.h>
#include <boost/thread/mutex.hpp>

class dc_corr_cc;

typedef boost::shared_ptr<dc_corr_cc> dc_corr_cc_sptr;


/*! \brief Return a shared_ptr to a new instance of dc_corr_cc.
 *  \param alpha The "speed" of averaging between 0.0 and 1.0
 *
 * This is effectively the public constructor for a new DC correction block.
 * To avoid accidental use of raw pointers, the dc_corr_cc constructor
 * is private.
 * make_dc_corr_cc is the public interface for creating new instances.
 */
dc_corr_cc_sptr make_dc_corr_cc(float alpha=0.1);


/*! \brief Long time average-based DC offset correction block.
 *  \ingroup DSP
 *
 * This block performs automatic gain control.
 * To be written...
 *
 */
class dc_corr_cc : public gr_sync_block
{
    friend dc_corr_cc_sptr make_dc_corr_cc(float alpha);

protected:
    dc_corr_cc(float alpha=0.1);

public:
    ~dc_corr_cc();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

private:
    boost::mutex  d_mutex;

    float d_alpha;
    float d_avg_i;  /*! Long time average of the I channel. */
    float d_avg_q;  /*! Long time average of the Q channel. */

    int d_cnt;
};


#endif /* CORRECT_IQ_CC_H */
