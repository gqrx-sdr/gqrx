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
#pragma once

#include <gnuradio/hier_block2.h>

class tx_modulator;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_modulator> tx_modulator_sptr;
#else
typedef std::shared_ptr<tx_modulator> tx_modulator_sptr;
#endif

/*! \brief Available TX modulations. */
enum tx_mod_mode {
    TX_MOD_AM  = 0,  /*!< Amplitude modulation (double side band with carrier). */
    TX_MOD_NFM = 1,  /*!< Narrow band FM. */
    TX_MOD_USB = 2,  /*!< Upper side band. */
    TX_MOD_LSB = 3   /*!< Lower side band. */
};

/*! \brief Return a shared_ptr to a new instance of tx_modulator.
 *  \param mode        The modulation.
 *  \param audio_rate  Sample rate of the audio input (and the complex output).
 *  \param max_dev     Peak deviation in Hz (NFM only).
 *  \param am_depth    Modulation depth between 0.0 and 1.0 (AM only).
 *
 * This is effectively the public constructor.
 */
tx_modulator_sptr make_tx_modulator(tx_mod_mode mode, float audio_rate,
                                    float max_dev = 2500.0f,
                                    float am_depth = 0.8f);

/*! \brief Audio to complex baseband modulator.
 *  \ingroup DSP
 *
 * Takes real audio and produces complex baseband at the same sample rate.
 * The audio is band limited and clipped so that the magnitude of the output
 * never exceeds 0.9. The output is meant to be resampled to the sample
 * rate of the transmitting device.
 */
class tx_modulator : public gr::hier_block2
{

public:
    tx_modulator(tx_mod_mode mode, float audio_rate, float max_dev,
                 float am_depth); // FIXME: should be private
    ~tx_modulator();

    tx_mod_mode mode() const { return d_mode; }

private:
    tx_mod_mode d_mode;
};
