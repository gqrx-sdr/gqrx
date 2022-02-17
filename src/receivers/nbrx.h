/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#ifndef NBRX_H
#define NBRX_H

#include <gnuradio/basic_block.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/complex_to_real.h>
#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"

class nbrx;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<nbrx> nbrx_sptr;
#else
typedef std::shared_ptr<nbrx> nbrx_sptr;
#endif

/*! \brief Public constructor of nbrx_sptr. */
nbrx_sptr make_nbrx(float quad_rate, float audio_rate);

/*! \brief Narrow band analog receiver
 *  \ingroup RX
 *
 * This block provides receiver for AM, narrow band FM and SSB modes.
 */
class nbrx : public receiver_base_cf
{
public:
    nbrx(float quad_rate, float audio_rate);
    virtual ~nbrx() { };

    bool start() override;
    bool stop() override;

    void set_filter(int low, int high, int tw) override;
    void set_offset(int offset) override;
    void set_cw_offset(int offset) override;

    /* Noise blanker */
    bool has_nb() override { return true; }
    void set_nb_on(int nbid, bool on) override;
    void set_nb_threshold(int nbid, float threshold) override;

    void set_demod(Modulations::idx new_demod) override;

    /* FM parameters */
    void set_fm_maxdev(float maxdev_hz) override;
    void set_fm_deemph(double tau) override;

    /* AM parameters */
    void set_am_dcr(bool enabled) override;

    /* AM-Sync parameters */
    void set_amsync_dcr(bool enabled) override;
    void set_amsync_pll_bw(float pll_bw) override;

private:
    bool   d_running;          /*!< Whether receiver is running or not. */

    rx_filter_sptr            filter;  /*!< Non-translating bandpass filter.*/

    rx_nb_cc_sptr             nb;         /*!< Noise blanker. */
    gr::blocks::complex_to_float::sptr  demod_raw;  /*!< Raw I/Q passthrough. */
    gr::blocks::complex_to_real::sptr   demod_ssb;  /*!< SSB demodulator. */
    rx_demod_fm_sptr          demod_fm;   /*!< FM demodulator. */
    rx_demod_am_sptr          demod_am;   /*!< AM demodulator. */
    rx_demod_amsync_sptr      demod_amsync;   /*!< AM-Sync demodulator. */
    resampler_ff_sptr         audio_rr0;  /*!< Audio resampler. */
    resampler_ff_sptr         audio_rr1;  /*!< Audio resampler. */

    gr::basic_block_sptr      demod;    // dummy pointer used for simplifying reconf
};

#endif // NBRX_H
