/* -*- c++ -*- */
/*
 * Copyright 2012 Joshua Roys KK4AFZ.
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
#ifndef RX_DEMOD_P25_H
#define RX_DEMOD_P25_H

#include <gr_complex_to_xxx.h>
#include <gr_diff_phasor_cc.h>
#include <gr_feedforward_agc_cc.h>
#include <gr_multiply_const_cc.h>
#include <gr_multiply_const_ff.h>
#include <repeater_gardner_costas_cc.h>

#include <gr_fir_filter_fff.h>
#include <gr_hier_block2.h>
#include <gr_msg_queue.h>
#include <gr_quadrature_demod_cf.h>
#include <op25_decoder_bf.h>
#include <op25_fsk4_demod_ff.h>
#include <op25_fsk4_slicer_fb.h>
#include <vector>
#include "dsp/resampler_xx.h"


class rx_demod_p25;


typedef boost::shared_ptr<rx_demod_p25> rx_demod_p25_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_demod_p25.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_demod_p25's constructor is private.
 * make_rx_dmod_p25 is the public interface for creating new instances.
 */
rx_demod_p25_sptr make_rx_demod_p25(float quad_rate, float audio_rate, float max_dev);


/*! \brief P25 demodulator.
 *  \ingroup DSP
 *
 * This class implements a P25 demodulator using OP25.
 *
 */
class rx_demod_p25 : public gr_hier_block2
{

public:
    rx_demod_p25(float quad_rate=48000.0, float audio_rate=48000.0, float max_dev=600.0); // FIXME: should be private
    ~rx_demod_p25();

private:
    /* GR blocks */
    /* C4FM */
    gr_quadrature_demod_cf_sptr   d_quad;          /*! The quadrature demodulator block. */
    gr_fir_filter_fff_sptr        d_symbol_filter; /*! Symbol filter. */
    op25_fsk4_demod_ff_sptr       d_demod_fsk4;    /*! C4FM/FSK4 demodulator block. */
    /* CQPSK */
    gr_feedforward_agc_cc_sptr    d_agc;           /*! AGC/prescaler to for Costas loop. */
    repeater_gardner_costas_cc_sptr d_clock;       /*! Clock/phase recovery. */
    gr_diff_phasor_cc_sptr        d_diffdec;       /*! Differential decoder. */
    gr_complex_to_arg_sptr        d_to_float;      /*! Complex to float. */
    gr_multiply_const_ff_sptr     d_rescale;       /*! Rescaler to {-3, -1, 1, 3} levels. */
    /* both */
    op25_fsk4_slicer_fb_sptr      d_slicer;        /*! FSK4 slicer block. */
    op25_decoder_bf_sptr          d_op25;          /*! P25 decoder block. */
    resampler_ff_sptr             d_audio_resamp;  /*! Audio resampler. */

    /* other parameters */
    float  d_quad_rate;        /*! Quadrature rate. */
    float  d_audio_rate;       /*! Audio rate. */
    float  d_max_dev;          /*! Max deviation. */
    gr_msg_queue_sptr d_queue; /*! Message queue to FSK4 demod. */

    /* FIR filter taps */
    std::vector<float> d_symbol_coeffs; /*! Symbol coefficients. */

};


#endif // RX_DEMOD_P25_H
