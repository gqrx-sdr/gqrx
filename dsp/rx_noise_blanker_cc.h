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
#ifndef RX_NB_CC_H
#define RX_NB_CC_H

#include <gr_sync_block.h>
#include <gr_complex.h>
#include <boost/thread/mutex.hpp>

class rx_nb_cc;

typedef boost::shared_ptr<rx_nb_cc> rx_nb_cc_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_nb_cc.
 *  \param sample_rate The samle rate (default = 96000).
 *  \param threshold Noise blanker threshold. Range 0.0 to 1.0 (TBC)
 *
 * This is effectively the public constructor for a new noise blanker block.
 * To avoid accidental use of raw pointers, the rx_nb_cc constructor is private.
 * make_rx_nb_cc is the public interface for creating new instances.
 */
rx_nb_cc_sptr make_rx_nb_cc(double sample_rate = 96000.0, float threshold = 0.5);


/*! \brief Noise blanker block.
 *  \ingroup DSP
 *
 * This block implements noise blanking filters based on the noise blanker code
 * from DTTSP.

 *
 */
class rx_nb_cc : public gr_sync_block
{
    friend rx_nb_cc_sptr make_rx_nb_cc(double sample_rate, float threshold);

protected:
    rx_nb_cc(double sample_rate, float threshold);

public:
    ~rx_nb_cc();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void set_sample_rate(double sample_rate) { d_sample_rate = sample_rate; }
    void set_nb1_on(bool nb1_on) { d_nb1_on = nb1_on; }
    void set_nb2_on(bool nb2_on) { d_nb2_on = nb2_on; }
    void set_threshold(float threshold) { d_threshold = threshold; }

private:
    boost::mutex  d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    bool   d_nb1_on;        /*! Current NB1 status (true/false). */
    bool   d_nb2_on;        /*! Current NB2 status (true/false). */
    double d_sample_rate;   /*! Current sample rate. */
    int    d_threshold;     /*! Current noise blanker threshols (0.0 to 1.0). */
};


#endif /* RX_NB_CC_H */
