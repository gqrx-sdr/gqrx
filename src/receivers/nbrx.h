/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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

#include <gnuradio/analog/simple_squelch_cc.h>
#include <gnuradio/basic_block.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/complex_to_real.h>
#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"
//#include "dsp/resampler_ff.h"
#include "dsp/resampler_xx.h"

class nbrx;

typedef boost::shared_ptr<nbrx> nbrx_sptr;

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
    /*! \brief Available demodulators. */
    enum nbrx_demod {
        NBRX_DEMOD_NONE = 0,  /*!< No demod. Raw I/Q to audio. */
        NBRX_DEMOD_AM   = 1,  /*!< Amplitude modulation. */
        NBRX_DEMOD_FM   = 2,  /*!< Frequency modulation. */
        NBRX_DEMOD_SSB  = 3,  /*!< Single Side Band. */
        NBRX_DEMOD_NUM  = 4   /*!< Included for convenience. */
    };

public:
    nbrx(float quad_rate, float audio_rate);
    virtual ~nbrx() { };

    bool start();
    bool stop();

    void set_quad_rate(float quad_rate);
    void set_audio_rate(float audio_rate);

    void set_filter(double low, double high, double tw);
    void set_cw_offset(double offset);

    float get_signal_level(bool dbfs);

    /* Noise blanker */
    bool has_nb() { return true; }
    void set_nb_on(int nbid, bool on);
    void set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    bool has_sql() { return true; }
    void set_sql_level(double level_db);
    void set_sql_alpha(double alpha);

    /* AGC */
    bool has_agc() { return true; }
    void set_agc_on(bool agc_on);
    void set_agc_hang(bool use_hang);
    void set_agc_threshold(int threshold);
    void set_agc_slope(int slope);
    void set_agc_decay(int decay_ms);
    void set_agc_manual_gain(int gain);

    void set_demod(int demod);

    /* FM parameters */
    bool has_fm() { return true; }
    void set_fm_maxdev(float maxdev_hz);
    void set_fm_deemph(double tau);

    /* AM parameters */
    bool has_am() { return true; }
    void set_am_dcr(bool enabled);

private:
    bool   d_running;          /*!< Whether receiver is running or not. */
    float  d_quad_rate;        /*!< Input sample rate. */
    int    d_audio_rate;       /*!< Audio output rate. */

    nbrx_demod                d_demod;    /*!< Current demodulator. */

    resampler_cc_sptr         iq_resamp;   /*!< Baseband resampler. */
    rx_filter_sptr            filter;  /*!< Non-translating bandpass filter.*/

    rx_nb_cc_sptr             nb;         /*!< Noise blanker. */
    rx_meter_c_sptr           meter;      /*!< Signal strength. */
    rx_agc_cc_sptr            agc;        /*!< Receiver AGC. */
    gr::analog::simple_squelch_cc::sptr sql;        /*!< Squelch. */
    gr::blocks::complex_to_float::sptr  demod_raw;  /*!< Raw I/Q passthrough. */
    gr::blocks::complex_to_real::sptr   demod_ssb;  /*!< SSB demodulator. */
    rx_demod_fm_sptr          demod_fm;   /*!< FM demodulator. */
    rx_demod_am_sptr          demod_am;   /*!< AM demodulator. */
    resampler_ff_sptr         audio_rr;   /*!< Audio resampler. */

    gr::basic_block_sptr      demod;    // dummy pointer used for simplifying reconf
};

#endif // NBRX_H
