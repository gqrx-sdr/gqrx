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
#include <math.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <dsp/rx_agc_xx.h>

rx_agc_cc_sptr make_rx_agc_cc(double sample_rate, bool agc_on, int threshold,
                              int manual_gain, int slope, int decay, bool use_hang)
{
    return gnuradio::get_initial_sptr(new rx_agc_cc(sample_rate, agc_on, threshold,
                                                    manual_gain, slope, decay,
                                                    use_hang));
}

/**
 * \brief Create receiver AGC object.
 *
 * Use make_rx_agc_cc() instead.
 */
rx_agc_cc::rx_agc_cc(double sample_rate, bool agc_on, int threshold,
                     int manual_gain, int slope, int decay, bool use_hang)
    : gr::sync_block ("rx_agc_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_agc_on(agc_on),
      d_sample_rate(sample_rate),
      d_threshold(threshold),
      d_manual_gain(manual_gain),
      d_slope(slope),
      d_decay(decay),
      d_use_hang(use_hang)
{
    d_agc = new CAgc();
    d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                         d_slope, d_decay, d_sample_rate);
}

rx_agc_cc::~rx_agc_cc()
{
    delete d_agc;
}

/**
 * \brief Receiver AGC work method.
 * \param mooutput_items
 * \param input_items
 * \param output_items
 */
int rx_agc_cc::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];

    boost::mutex::scoped_lock lock(d_mutex);
    d_agc->ProcessData(noutput_items, in, out);

    return noutput_items;
}

/**
 * \brief Enable or disable AGC.
 * \param agc_on Whether AGC should be endabled.
 *
 * When AGC is disabled a fixed gain is used.
 *
 * \sa set_manual_gain()
 */
void rx_agc_cc::set_agc_on(bool agc_on)
{
    if (agc_on != d_agc_on) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_agc_on = agc_on;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Set AGC sample rate.
 * \param sample_rate The sample rate.
 *
 * The AGC uses knowledge about the sample rate to calculate various delays and
 * time constants.
 */
void rx_agc_cc::set_sample_rate(double sample_rate)
{
    if (sample_rate != d_sample_rate) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_sample_rate = sample_rate;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Set new AGC threshold.
 * \param threshold The new threshold between -160 and 0dB.
 *
 * The threshold specifies AGC "knee" in dB when the AGC is active.
 */
void rx_agc_cc::set_threshold(int threshold)
{
    if ((threshold != d_threshold) && (threshold >= -160) && (threshold <= 0)) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_threshold = threshold;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Set new manual gain.
 * \param gain The new manual gain between 0 and 100dB.
 *
 * The manual gain is used when AGC is switched off.
 *
 * \sa set_agc_on()
 */
void rx_agc_cc::set_manual_gain(int gain)
{
    if ((gain != d_manual_gain) && (gain >= 0) && (gain <= 100)) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_manual_gain = gain;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Set AGC slope factor.
 * \param slope The new slope factor between 0 and 10dB.
 *
 * The slope factor specifies dB reduction in output at knee from maximum output level
 */
void rx_agc_cc::set_slope(int slope)
{
    if ((slope != d_slope) && (slope >= 0) && (slope <= 10)) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_slope = slope;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Set AGC decay time.
 * \param decay The new AGC decay time between 20 to 5000 ms.
 */
void rx_agc_cc::set_decay(int decay)
{
    if ((decay != d_decay) && (decay >= 20) && (decay <= 5000)) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_decay = decay;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}

/**
 * \brief Enable/disable AGC hang.
 * \param use_hang Whether to use hang or not.
 */
void rx_agc_cc::set_use_hang(bool use_hang)
{
    if (use_hang != d_use_hang) {
        boost::mutex::scoped_lock lock(d_mutex);
        d_use_hang = use_hang;
        d_agc->SetParameters(d_agc_on, d_use_hang, d_threshold, d_manual_gain,
                             d_slope, d_decay, d_sample_rate);
    }
}
