/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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

rx_agc_cc_sptr make_rx_agc_cc(double sample_rate, bool agc_on, int target_level,
                              int manual_gain, int max_gain, int attack, int decay, int hang)
{
    return gnuradio::get_initial_sptr(new rx_agc_cc(sample_rate, agc_on, target_level,
                                                    manual_gain, max_gain, attack, decay,
                                                    hang));
}

/**
 * \brief Create receiver AGC object.
 *
 * Use make_rx_agc_cc() instead.
 */
rx_agc_cc::rx_agc_cc(double sample_rate, bool agc_on, int target_level,
                              int manual_gain, int max_gain, int attack, int decay, int hang)
    : gr::sync_block ("rx_agc_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_agc_on(agc_on),
      d_sample_rate(sample_rate),
      d_target_level(target_level),
      d_manual_gain(manual_gain),
      d_max_gain(max_gain),
      d_attack(attack),
      d_decay(decay),
      d_hang(hang)
{
    d_agc = new CAgc();
    reconfigure();
}

rx_agc_cc::~rx_agc_cc()
{
    delete d_agc;
}

void rx_agc_cc::reconfigure()
{
    d_agc->SetParameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang);
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

    std::lock_guard<std::mutex> lock(d_mutex);
    d_agc->ProcessData(out, in, noutput_items);

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
        std::lock_guard<std::mutex> lock(d_mutex);
        d_agc_on = agc_on;
        reconfigure();
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
        std::lock_guard<std::mutex> lock(d_mutex);
        d_sample_rate = sample_rate;
        reconfigure();
    }
}

/**
 * \brief Set new AGC target level.
 * \param threshold The new target level between -160 and 0dB.
 *
 * Maximum output signal lenvel in dB..
 */
void rx_agc_cc::set_target_level(int target_level)
{
    if ((target_level != d_target_level) && (target_level >= -160) && (target_level <= 0)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_target_level = target_level;
        reconfigure();
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
    if ((gain != d_manual_gain) && (gain >= -160) && (gain <= 160)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_manual_gain = gain;
        reconfigure();
    }
}

/**
 * \brief Set new max gain.
 * \param gain The new max gain between 0 and 100dB.
 *
 * Limits maximum AGC gain to reduce noise.
 *
 * \sa set_agc_on()
 */
void rx_agc_cc::set_max_gain(int gain)
{
    if ((gain != d_max_gain) && (gain >= 0) && (gain <= 160)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_max_gain = gain;
        reconfigure();
    }
}

/**
 * \brief Set AGC attack time.
 * \param decay The new AGC attack time between 20 to 5000 ms.
 *
 * Sets length of the delay buffer
 *
 */
void rx_agc_cc::set_attack(int attack)
{
    if ((attack != d_attack) && (attack >= 20) && (attack <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_attack = attack;
        reconfigure();
    }
}

/**
 * \brief Set AGC decay time.
 * \param decay The new AGC decay time between 20 to 5000 ms.
 */
void rx_agc_cc::set_decay(int decay)
{
    if ((decay != d_decay) && (decay >= 20) && (decay <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_decay = decay;
        reconfigure();
    }
}

/**
 * \brief Set AGC hang time between 0 to 5000 ms.
 * \param hang Time to keep AGC gain at constant level after the peak.
 */
void rx_agc_cc::set_hang(int hang)
{
    if ((hang != d_hang) && (hang >= 0) && (hang <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_hang = hang;
        reconfigure();
    }
}
