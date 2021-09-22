/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef RECEIVER_H
#define RECEIVER_H

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#endif

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/wavfile_source.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>

#include <memory>
#include <string>
#include <vector>

#include <QDebug>

#include "dsp/correct_iq_cc.h"
#include "dsp/downconverter.h"
#include "dsp/filter/fir_decim.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"
#include "dsp/rx_fft.h"
#include "dsp/sniffer_f.h"
#include "dsp/resampler_xx.h"
#include "interfaces/udp_sink_f.h"
#include "receivers/receiver_base.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif

/** Flag used to indicate success or failure of an operation */
enum rx_status {
    STATUS_OK    = 0, /*!< Operation was successful. */
    STATUS_ERROR = 1  /*!< There was an error. */
};

/** Available demodulators. */
enum rx_demod {
    RX_DEMOD_OFF   = 0,  /*!< No receiver. */
    RX_DEMOD_NONE  = 1,  /*!< No demod. Raw I/Q to audio. */
    RX_DEMOD_AM    = 2,  /*!< Amplitude modulation. */
    RX_DEMOD_NFM   = 3,  /*!< Frequency modulation. */
    RX_DEMOD_WFM_M = 4,  /*!< Frequency modulation (wide, mono). */
    RX_DEMOD_WFM_S = 5,  /*!< Frequency modulation (wide, stereo). */
    RX_DEMOD_WFM_S_OIRT = 6,  /*!< Frequency modulation (wide, stereo oirt). */
    RX_DEMOD_SSB   = 7,  /*!< Single Side Band. */
    RX_DEMOD_AMSYNC = 8  /*!< Amplitude modulation (synchronous demod). */
};

/** Supported receiver types. */
enum rx_chain {
    RX_CHAIN_NONE  = 0,   /*!< No receiver, just spectrum analyzer. */
    RX_CHAIN_NBRX  = 1,   /*!< Narrow band receiver (AM, FM, SSB). */
    RX_CHAIN_WFMRX = 2    /*!< Wide band FM receiver (for broadcast). */
};

/** Filter shape (convenience wrappers for "transition width"). */
enum rx_filter_shape {
    FILTER_SHAPE_SOFT = 0,   /*!< Soft: Transition band is TBD of width. */
    FILTER_SHAPE_NORMAL = 1, /*!< Normal: Transition band is TBD of width. */
    FILTER_SHAPE_SHARP = 2   /*!< Sharp: Transition band is TBD of width. */
};

class subreceiver
{
public:
    typedef std::shared_ptr<subreceiver> SharedPointer;

    subreceiver(
            gr::top_block_sptr tb,
            gr::basic_block_sptr src,
            const std::string audio_device,
            const int idx,
            int d_ddc_decim,
            int d_decim_rate,
            int d_quad_rate,
            int d_audio_rate
    );
    ~subreceiver();

public:
    /* General */
    void        set_idx(size_t next_idx) {
        qInfo() << "subreceiver" << idx << "gets new" << next_idx;
        idx = next_idx;
    }

    /* I/O control */
    void        set_output_device(const std::string device, const int d_audio_rate);
    void        set_input_rate(const int d_ddc_decim, const int d_decim_rate, const int d_quad_rate);

    /* Demodulation type */
    rx_status   set_demod(rx_demod demod, bool force, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate);
    rx_demod    get_demod() const {
        return d_demod;
    };

    /* Frequency control */
    rx_status   set_filter(double low, double high, rx_filter_shape shape);
    rx_status   set_filter_offset(double offset_hz);
    double      get_filter_offset(void) const;
    rx_status   set_cw_offset(double offset_hz);
    double      get_cw_offset(void) const;

    /* Noise blanker */
    rx_status   set_nb_on(int nbid, bool on);
    rx_status   set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    rx_status   set_sql_level(double level_db);
    rx_status   set_sql_alpha(double alpha);

    /* AGC */
    rx_status   set_agc_on(bool agc_on);
    rx_status   set_agc_hang(bool use_hang);
    rx_status   set_agc_threshold(int threshold);
    rx_status   set_agc_slope(int slope);
    rx_status   set_agc_decay(int decay_ms);
    rx_status   set_agc_manual_gain(int gain);

    /* FM parameters */
    rx_status   set_fm_maxdev(float maxdev_hz);
    rx_status   set_fm_deemph(double tau);

    /* AM parameters */
    rx_status   set_am_dcr(bool enabled);

    /* AM-Sync parameters */
    rx_status   set_amsync_dcr(bool enabled);
    rx_status   set_amsync_pll_bw(float pll_bw);

    /* Audio parameters */
    rx_status   set_af_gain(float gain_db);

    /* Data inspection */
    float       get_signal_pwr(bool dbfs) const {
        return rx->get_signal_level(dbfs);
    }
    void        get_audio_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize) const {
        audio_fft->get_fft_data(fftPoints, fftsize);
    }

    /* Audio Record/Playback */
//    rx_status      start_audio_recording(const std::string filename);
//    rx_status      stop_audio_recording();
//    rx_status      start_audio_playback(const std::string filename);
//    rx_status      stop_audio_playback();

//    rx_status      start_udp_streaming(const std::string host, int port, bool stereo);
//    rx_status      stop_udp_streaming();

    /* I/Q recording and playback */
//    rx_status      start_iq_recording(const std::string filename);
//    rx_status      stop_iq_recording();
//    rx_status      seek_iq_file(long pos);

    /* sample sniffer */
//    rx_status      start_sniffer(unsigned int samplrate, int buffsize);
//    rx_status      stop_sniffer();
//    void                get_sniffer_data(float * outbuff, unsigned int &num);
//    bool                is_recording_audio(void) const { return d_recording_wav; }
//    bool                is_snifffer_active(void) const { return d_sniffer_active; }

    /* rds functions */
//    void        get_rds_data(std::string &outbuff, int &num);
//    void        start_rds_decoder(void);
//    void        stop_rds_decoder();
//    bool        is_rds_decoder_active(void) const;
//    void        reset_rds_parser(void);

    /* Plumbing */
    void connect_all(rx_chain type, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate);

private:
    gr::top_block_sptr tb;                 /*!< Top Block */
    int                idx;                /*!< This sub-receiver index */

    double             d_filter_offset;    /*!< Current filter offset */
    double             d_cw_offset;        /*!< CW offset */
    bool               d_recording_iq;     /*!< Whether we are recording I/Q file. */
    bool               d_recording_wav;    /*!< Whether we are recording WAV file. */
    bool               d_sniffer_active;   /*!< Only one data decoder allowed. */

    rx_demod           d_demod;            /*!< Current demodulator. */

    downconverter_cc_sptr               ddc     ;         /*!< Digital down-converter for demod chain. */

    gr::blocks::multiply_const_ff::sptr audio_gain0;      /*!< Audio gain block. */
    gr::blocks::multiply_const_ff::sptr audio_gain1;      /*!< Audio gain block. */

    gr::blocks::file_sink::sptr         iq_sink;          /*!< I/Q file sink. */

//    gr::blocks::wavfile_sink::sptr      wav_sink;         /*!< WAV file sink for recording. */
//    gr::blocks::wavfile_source::sptr    wav_src;          /*!< WAV file source for playback. */
//    gr::blocks::null_sink::sptr         audio_null_sink0; /*!< Audio null sink used during playback. */
//    gr::blocks::null_sink::sptr         audio_null_sink1; /*!< Audio null sink used during playback. */

//    udp_sink_f_sptr   audio_udp_sink;                     /*!< UDP sink to stream audio over the network. */
//    sniffer_f_sptr    sniffer;                            /*!< Sample sniffer for data decoders. */
//    resampler_ff_sptr sniffer_rr;                         /*!< Sniffer resampler. */

    receiver_base_cf_sptr     rx;          /*!< receive hierblock. */
    rx_fft_f_sptr             audio_fft;   /*!< Audio FFT block. */

#ifdef WITH_PULSEAUDIO
    pa_sink_sptr              audio_snk;  /*!< Pulse audio sink. */
#elif WITH_PORTAUDIO
    portaudio_sink_sptr       audio_snk;  /*!< portaudio sink */
#else
    gr::audio::sink::sptr     audio_snk;  /*!< gr audio sink */
#endif
};


/**
 * @defgroup DSP Digital signal processing library based on GNU Radio
 */

/**
 * @brief Top-level receiver class.
 * @ingroup DSP
 *
 * This class encapsulates the GNU Radio flow graph for the receiver.
 * Front-ends should only control the receiver through the interface provided
 * by this class.
 */
class receiver
{

public:
    typedef std::shared_ptr<receiver> SharedPointer;

    receiver(const std::string input_device="",
             const std::string audio_device="",
             unsigned int decimation=1);
    ~receiver();

    void            start();
    void            stop();
    void            set_input_device(const std::string device);

    /* Input device control */
    std::vector<std::string> get_antennas(void) const;
    void                     set_antenna(const std::string &antenna);

    double          set_input_rate(double rate);
    double          get_input_rate(void) const { return d_input_rate; }

    unsigned int    set_input_decim(unsigned int decim);
    unsigned int    get_input_decim(void) const { return d_decim; }

    double          get_quad_rate(void) const { return d_input_rate / (double)d_decim; }

    double          set_analog_bandwidth(double bw);
    double          get_analog_bandwidth(void) const;

    void            set_iq_swap(bool reversed);
    bool            get_iq_swap(void) const;

    void            set_dc_cancel(bool enable);
    bool            get_dc_cancel(void) const;

    void            set_iq_balance(bool enable);
    bool            get_iq_balance(void) const;

    rx_status       set_rf_freq(double freq_hz);
    double          get_rf_freq(void);
    rx_status       get_rf_range(double *start, double *stop, double *step);

    std::vector<std::string>  get_gain_names();
    rx_status                 get_gain_range(std::string &name, double *start, double *stop, double *step) const;
    rx_status                 set_auto_gain(bool automatic);
    rx_status                 set_gain(std::string name, double value);
    double                    get_gain(std::string name) const;

    rx_status                 set_freq_corr(double ppm);

    /* Receiver data for visualisation */
    void        set_iq_fft_size(int newsize);
    void        set_iq_fft_window(int window_type);
    void        get_iq_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize);

    /* Flowgraph configuration */
    void        begin_reconfigure();
    void        complete_reconfigure();

    /* Sub-receivers control */
    size_t      add_rx();
    void        remove_rx(size_t idx);

    rx_status   set_filter_offset(const size_t idx, double offset_hz);
    double      get_filter_offset(const size_t idx) const;

    rx_status   set_cw_offset(const size_t idx, double offset_hz);
    double      get_cw_offset(const size_t idx) const;

    rx_status   set_filter(const size_t idx, double low, double high, rx_filter_shape shape);
    rx_status   set_nb_on(const size_t idx, int nbid, bool on);
    rx_status   set_nb_threshold(const size_t idx, int nbid, float threshold);
    rx_status   set_sql_level(const size_t idx, double level_d);
    rx_status   set_sql_alpha(const size_t idx, double alpha);
    rx_status   set_agc_on(const size_t idx, bool agc_on);
    rx_status   set_agc_hang(const size_t idx, bool use_hang);
    rx_status   set_agc_threshold(const size_t idx, int threshold);
    rx_status   set_agc_slope(const size_t idx, int slope);
    rx_status   set_agc_decay(const size_t idx, int decay_ms);
    rx_status   set_agc_manual_gain(const size_t idx, int gain);
    rx_status   set_demod(const size_t idx, rx_demod demod, bool force = false);
    rx_status   set_fm_maxdev(const size_t idx, float maxdev_hz);
    rx_status   set_fm_deemph(const size_t idx, double tau);
    rx_status   set_am_dcr(const size_t idx, bool enabled);
    rx_status   set_amsync_dcr(const size_t idx, bool enabled);
    rx_status   set_amsync_pll_bw(const size_t idx, float pll_bw);
    rx_status   set_af_gain(const size_t idx, float gain_db);

    /* Sub-receiver data for visualisation */
    float       get_signal_pwr(const size_t idx, bool dbfs) const;
    void        get_audio_fft_data(const size_t idx, std::complex<float>* fftPoints, unsigned int &fftsize);

    void        set_output_device(const std::string device);

private:
    void        connect_all(); /*!< Return the block to use as subreciever source */

private:
    bool          d_running;          /*!< Whether receiver is running or not. */
    double        d_input_rate;       /*!< Input sample rate. */
    double        d_decim_rate;       /*!< Rate after decimation (input_rate / decim) */
    double        d_quad_rate;        /*!< Quadrature rate (after down-conversion) */
    double        d_audio_rate;       /*!< Audio output rate. */
    unsigned int  d_decim;            /*!< input decimation. */
    unsigned int  d_ddc_decim;        /*!< Down-conversion decimation. */
    double        d_rf_freq;          /*!< Current RF frequency. */

    bool          d_iq_rev;           /*!< Whether I/Q is reversed or not. */
    bool          d_dc_cancel;        /*!< Enable automatic DC removal. */
    bool          d_iq_balance;       /*!< Enable automatic IQ balance. */

    std::string   input_devstr;       /*!< Current input device string. */
    std::string   output_devstr;      /*!< Current output device string. */

    gr::top_block_sptr        tb;          /*!< The GNU Radio top block. */

    osmosdr::source::sptr     src;         /*!< Real time I/Q source. */
    fir_decim_cc_sptr         input_decim; /*!< Input decimator. */
    gr::basic_block_sptr      subrxsrc;    /*!< Input to sub receivers */

    dc_corr_cc_sptr           dc_corr;     /*!< DC corrector block. */
    iq_swap_cc_sptr           iq_swap;     /*!< I/Q swapping block. */

    rx_fft_c_sptr             iq_fft;      /*!< Baseband FFT block. */

    //! Get a path to a file containing random bytes
    static std::string get_random_file(void);

    /* Sub-receivers all demodulating the same input */
    std::vector<subreceiver::SharedPointer> subrx;
};

#endif // RECEIVER_H
