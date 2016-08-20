/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
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
#ifndef RX_AGC_XX_H
#define RX_AGC_XX_H

#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#include <boost/thread/mutex.hpp>
#include <dsp/agc_impl.h>

class rx_agc_cc;

typedef boost::shared_ptr<rx_agc_cc> rx_agc_cc_sptr;


/**
 * \brief Return a shared_ptr to a new instance of rx_agc_cc.
 * \param sample_rate The samle rate (default = 96000).
 * \param agc_on      Whether AGC should be ON (default = true).
 * \param threshold   AGC Knee in dB if AGC is active. Range -160 to 0 dB.
 * \param manual_gain Manual gain when AGC is OFF. Range 0 to 100 dB.
 * \param slope       AGC slope factor. Specifies dB reduction in output at
 *                    knee from maximum output level. Range 0 to 10 dB
 * \param decay       AGC decay time in milliseconds. Range 20 to 5000. This
 *                    parameter determines whether AGC is fast, slow or medium.
 * \param use_hang    Whether AGC should "hang" before starting to decay.
 *
 * This is effectively the public constructor for a new AGC block.
 * To avoid accidental use of raw pointers, the rx_agc_cc constructor is private.
 * make_rx_agc_cc is the public interface for creating new instances.
 */
rx_agc_cc_sptr make_rx_agc_cc(double sample_rate, bool agc_on, int threshold,
                              int manual_gain, int slope, int decay,
                              bool use_hang);

/**
 * \brief Experimental AGC block for analog voice modes (AM, SSB, CW).
 * \ingroup DSP
 *
 * This block performs automatic gain control.
 * To be written...
 */
class rx_agc_cc : public gr::sync_block
{
    friend rx_agc_cc_sptr make_rx_agc_cc(double sample_rate, bool agc_on,
                                         int threshold, int manual_gain,
                                         int slope, int decay, bool use_hang);

protected:
    rx_agc_cc(double sample_rate, bool agc_on, int threshold, int manual_gain,
              int slope, int decay, bool use_hang);

public:
    ~rx_agc_cc();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void set_agc_on(bool agc_on);
    void set_sample_rate(double sample_rate);
    void set_threshold(int threshold);
    void set_manual_gain(int gain);
    void set_slope(int slope);
    void set_decay(int decay);
    void set_use_hang(bool use_hang);

private:
    CAgc           *d_agc;
    boost::mutex    d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    bool            d_agc_on;        /*! Current AGC status (true/false). */
    double          d_sample_rate;   /*! Current sample rate. */
    int             d_threshold;     /*! Current AGC threshold (-160...0 dB). */
    int             d_manual_gain;   /*! Current gain when AGC is OFF. */
    int             d_slope;         /*! Current AGC slope (0...10 dB). */
    int             d_decay;         /*! Current AGC decay (20...5000 ms). */
    bool            d_use_hang;      /*! Current AGC hang status (true/false). */
};

#endif /* RX_AGC_XX_H */
