/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <cmath>
#include <vector>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/rail_ff.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>

#include "dsp/tx_modulator.h"

#define TX_AUDIO_LOW    300.0   /* Lower edge of the transmitted audio */
#define TX_SSB_HIGH     2700.0  /* Upper edge of the transmitted audio (SSB) */
#define TX_ENV_HIGH     3000.0  /* Upper edge of the transmitted audio (AM, FM) */
#define TX_OUT_LEVEL    0.9f    /* Maximum magnitude of the output */

/* Create a new instance of tx_modulator and return a shared_ptr. */
tx_modulator_sptr make_tx_modulator(tx_mod_mode mode, float audio_rate,
                                    float max_dev, float am_depth)
{
    return gnuradio::get_initial_sptr(new tx_modulator(mode, audio_rate,
                                                       max_dev, am_depth));
}

tx_modulator::tx_modulator(tx_mod_mode mode, float audio_rate, float max_dev,
                           float am_depth)
    : gr::hier_block2("tx_modulator",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_mode(mode)
{
    if (am_depth < 0.0f)
        am_depth = 0.0f;
    else if (am_depth > 1.0f)
        am_depth = 1.0f;

    switch (d_mode)
    {
    case TX_MOD_USB:
    case TX_MOD_LSB:
    {
        /* A one-sided complex band pass filter turns the real audio into a
         * single side band signal. The gain of 2 compensates for the energy
         * that is removed with the unwanted side of the real spectrum.
         */
        double low = (d_mode == TX_MOD_USB) ? TX_AUDIO_LOW : -TX_SSB_HIGH;
        double high = (d_mode == TX_MOD_USB) ? TX_SSB_HIGH : -TX_AUDIO_LOW;
        std::vector<gr_complex> taps =
            gr::filter::firdes::complex_band_pass(2.0 * TX_OUT_LEVEL,
                                                  audio_rate, low, high, 300.0);
        auto clip = gr::analog::rail_ff::make(-1.0, 1.0);
        auto ssb = gr::filter::fir_filter_fcc::make(1, taps);

        connect(self(), 0, clip, 0);
        connect(clip, 0, ssb, 0);
        connect(ssb, 0, self(), 0);
        break;
    }

    case TX_MOD_NFM:
    {
        std::vector<float> taps =
            gr::filter::firdes::band_pass(1.0, audio_rate, TX_AUDIO_LOW,
                                          TX_ENV_HIGH, 200.0);
        auto filt = gr::filter::fir_filter_fff::make(1, taps);
        auto clip = gr::analog::rail_ff::make(-1.0, 1.0);
        auto fm = gr::analog::frequency_modulator_fc::make(
            2.0 * M_PI * max_dev / audio_rate);

        connect(self(), 0, filt, 0);
        connect(filt, 0, clip, 0);
        connect(clip, 0, fm, 0);
        connect(fm, 0, self(), 0);
        break;
    }

    case TX_MOD_AM:
    default:
    {
        /* envelope = out_level * (1 + depth * audio) / (1 + depth) */
        std::vector<float> taps =
            gr::filter::firdes::band_pass(1.0, audio_rate, TX_AUDIO_LOW,
                                          TX_ENV_HIGH, 200.0);
        auto filt = gr::filter::fir_filter_fff::make(1, taps);
        auto clip = gr::analog::rail_ff::make(-1.0, 1.0);
        auto depth = gr::blocks::multiply_const_ff::make(am_depth);
        /* rail_ff clamped to [1, 1] outputs a constant 1.0 (the carrier) */
        auto one = gr::analog::rail_ff::make(1.0, 1.0);
        auto carrier = gr::blocks::add_ff::make();
        auto level = gr::blocks::multiply_const_ff::make(TX_OUT_LEVEL / (1.0f + am_depth));
        auto to_c = gr::blocks::float_to_complex::make(1);

        connect(self(), 0, filt, 0);
        connect(filt, 0, clip, 0);
        connect(clip, 0, depth, 0);
        connect(clip, 0, one, 0);
        connect(depth, 0, carrier, 0);
        connect(one, 0, carrier, 1);
        connect(carrier, 0, level, 0);
        connect(level, 0, to_c, 0);
        connect(to_c, 0, self(), 0);
        break;
    }
    }
}

tx_modulator::~tx_modulator()
{
}
