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
#include <gnuradio/blocks/add_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#endif

#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/null_sink.h>
//temporary workaround to make add_ff happy
#include <gnuradio/blocks/null_source.h>
//emd workaround
#include <gnuradio/blocks/wavfile_source.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <string>

#include "dsp/correct_iq_cc.h"
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

    /** Flag used to indicate success or failure of an operation */
    enum status {
        STATUS_OK    = 0, /*!< Operation was successful. */
        STATUS_ERROR = 1  /*!< There was an error. */
    };

    /** Supported receiver types. */
    enum rx_chain {
        RX_CHAIN_NONE  = 0,   /*!< No receiver, just spectrum analyzer. */
        RX_CHAIN_NBRX  = 1,   /*!< Narrow band receiver (AM, FM, SSB). */
        RX_CHAIN_WFMRX = 2    /*!< Wide band FM receiver (for broadcast). */
    };

    /** Filter shape (convenience wrappers for "transition width"). */
    typedef Modulations::filter_shape filter_shape;

    typedef std::function<void(std::string, bool)> audio_rec_event_handler_t;

    static const unsigned int DEFAULT_FFT_SIZE = 8192;

    receiver(const std::string input_device="",
             const std::string audio_device="",
             unsigned int decimation=1);
    ~receiver();

    void        start();
    void        stop();
    void        set_input_device(const std::string device);
    void        set_output_device(const std::string device);

    std::vector<std::string> get_antennas(void) const;
    void        set_antenna(const std::string &antenna);

    double      set_input_rate(double rate);
    double      get_input_rate(void) const { return d_input_rate; }

    unsigned int    set_input_decim(unsigned int decim);
    unsigned int    get_input_decim(void) const { return d_decim; }

    double      get_quad_rate(void) const {
        return d_input_rate / (double)d_decim;
    }

    double      set_analog_bandwidth(double bw);
    double      get_analog_bandwidth(void) const;

    void        set_iq_swap(bool reversed);
    bool        get_iq_swap(void) const;

    void        set_dc_cancel(bool enable);
    bool        get_dc_cancel(void) const;

    void        set_iq_balance(bool enable);
    bool        get_iq_balance(void) const;

    status      set_rf_freq(double freq_hz);
    double      get_rf_freq(void);
    status      get_rf_range(double *start, double *stop, double *step);

    std::vector<std::string>    get_gain_names();
    status      get_gain_range(std::string &name, double *start, double *stop,
                               double *step) const;
    status      set_auto_gain(bool automatic);
    status      set_gain(std::string name, double value);
    double      get_gain(std::string name) const;

    int         add_rx();
    int         get_rx_count();
    int         delete_rx();
    status      select_rx(int no);
    status      fake_select_rx(int no);
    int         get_current();
    vfo::sptr   get_current_vfo();
    vfo::sptr   get_vfo(int n);
    vfo::sptr   find_vfo(int64_t freq);
    std::vector<vfo::sptr> get_vfos();

    status      set_filter_offset(double offset_hz);
    status      set_filter_offset(int rx_index, double offset_hz);
    double      get_filter_offset(void) const;
    void        set_freq_lock(bool on, bool all = false);
    bool        get_freq_lock();

    status      set_cw_offset(double offset_hz);
    double      get_cw_offset(void) const;
    status      set_filter(int low, int high, filter_shape shape);
    status      get_filter(int &low, int &high, filter_shape &shape);
    status      set_freq_corr(double ppm);
    float       get_signal_pwr() const;
    void        set_iq_fft_size(int newsize);
    unsigned int iq_fft_size(void) const;
    void        set_iq_fft_window(int window_type, bool normalize_energy);
    int         get_iq_fft_data(float* fftPoints);
    int         get_audio_fft_data(float* fftPoints);
    unsigned int audio_fft_size(void) const;

    /* Noise blanker */
    status      set_nb_on(int nbid, bool on);
    bool        get_nb_on(int nbid);
    status      set_nb_threshold(int nbid, float threshold);
    float       get_nb_threshold(int nbid);

    /* Squelch parameter */
    status      set_sql_level(float level_db);
    status      set_sql_level(float level_offset, bool global, bool relative);
    double      get_sql_level();
    status      set_sql_alpha(double alpha);
    double      get_sql_alpha();

    /* AGC */
    status      set_agc_on(bool agc_on);
    bool        get_agc_on();
    status      set_agc_target_level(int target_level);
    int         get_agc_target_level();
    status      set_agc_manual_gain(float gain);
    float       get_agc_manual_gain();
    status      set_agc_max_gain(int gain);
    int         get_agc_max_gain();
    status      set_agc_attack(int attack_ms);
    int         get_agc_attack();
    status      set_agc_decay(int decay_ms);
    int         get_agc_decay();
    status      set_agc_hang(int hang_ms);
    int         get_agc_hang();
    status      set_agc_panning(int panning);
    int         get_agc_panning();
    status      set_agc_panning_auto(bool mode);
    bool        get_agc_panning_auto();
    status      set_agc_mute(bool agc_mute);
    bool        get_agc_mute();
    float       get_agc_gain();

    /* Mute */
    status      set_mute(bool mute);
    bool        get_mute();

    /* Demod */
    status      set_demod_locked(Modulations::idx demod, int old_idx = -1);
    status      set_demod(Modulations::idx demod, int old_idx = -1);
    Modulations::idx get_demod() {return rx[d_current]->get_demod();}
    status      reconnect_all(bool force = false);

    /* FM parameters */
    status      set_fm_maxdev(float maxdev_hz);
    float       get_fm_maxdev();
    status      set_fm_deemph(double tau);
    double      get_fm_deemph();

    /* AM parameters */
    status      set_am_dcr(bool enabled);
    bool        get_am_dcr();

    /* AM-Sync parameters */
    status      set_amsync_dcr(bool enabled);
    bool        get_amsync_dcr();
    status      set_amsync_pll_bw(float pll_bw);
    float       get_amsync_pll_bw();

    /* Audio parameters */
    status      set_audio_rec_dir(const std::string dir);
    std::string get_audio_rec_dir();
    status      set_audio_rec_sql_triggered(const bool enabled);
    bool        get_audio_rec_sql_triggered();
    status      set_audio_rec_min_time(const int time_ms);
    int         get_audio_rec_min_time();
    status      set_audio_rec_max_gap(const int time_ms);
    int         get_audio_rec_max_gap();
    status      start_audio_recording();
    status      stop_audio_recording();
    std::string get_last_audio_filename();
    status      start_audio_playback(const std::string filename);
    status      stop_audio_playback();

    /* UDP  streaming */
    status      set_udp_host(std::string host);
    std::string get_udp_host();
    status      set_udp_port(int port);
    int         get_udp_port();
    status      set_udp_stereo(bool stereo);
    bool        get_udp_stereo();
    status      set_udp_streaming(bool streaming);
    bool        get_udp_streaming();

    /* I/Q recording and playback */
    status      start_iq_recording(const std::string filename);
    status      stop_iq_recording();
    status      seek_iq_file(long pos);
    bool        is_playing_iq() const { return input_devstr.find("file=") != std::string::npos; }

    /* sample sniffer */
    status      start_sniffer(unsigned int samplrate, int buffsize);
    status      stop_sniffer();
    void        get_sniffer_data(float * outbuff, unsigned int &num);

    bool        is_recording_audio(void) const { return rx[d_current]->get_audio_recording(); }
    bool        is_snifffer_active(void) const { return d_sniffer_active; }

    /* rds functions */
    void        get_rds_data(std::string &outbuff, int &num);
    void        start_rds_decoder(void);
    void        stop_rds_decoder();
    bool        is_rds_decoder_active(void) const;
    void        reset_rds_parser(void);

    /* utility functions */
    static std::string escape_filename(std::string filename);
    template <typename T> void set_audio_rec_event_handler(T handler)
    {
        d_audio_rec_event_handler = handler;
    }

private:
    void        connect_all();
    void        connect_rx();
    void        connect_rx(int n);
    void        disconnect_rx();
    void        disconnect_rx(int n);
    void        foreground_rx();
    void        background_rx();
    status      connect_iq_recorder();

private:
    int         d_current;          /*!< Current selected demodulator. */
    int         d_active;           /*!< Active demodulator count. */
    bool        d_running;          /*!< Whether receiver is running or not. */
    double      d_input_rate;       /*!< Input sample rate. */
    double      d_decim_rate;       /*!< Rate after decimation (input_rate / decim) */
    double      d_audio_rate;       /*!< Audio output rate. */
    unsigned int    d_decim;        /*!< input decimation. */
    double      d_rf_freq;          /*!< Current RF frequency. */
    bool        d_recording_iq;     /*!< Whether we are recording I/Q file. */
    bool        d_sniffer_active;   /*!< Only one data decoder allowed. */
    bool        d_iq_rev;           /*!< Whether I/Q is reversed or not. */
    bool        d_dc_cancel;        /*!< Enable automatic DC removal. */
    bool        d_iq_balance;       /*!< Enable automatic IQ balance. */
    bool        d_mute;             /*!< Enable audio mute. */

    std::string input_devstr;       /*!< Current input device string. */
    std::string output_devstr;      /*!< Current output device string. */

    gr::basic_block_sptr iq_src;    /*!< Points to the block, connected to rx[]. */

    gr::top_block_sptr         tb;        /*!< The GNU Radio top block. */

    osmosdr::source::sptr     src;       /*!< Real time I/Q source. */
    fir_decim_cc_sptr         input_decim;      /*!< Input decimator. */
    std::vector<receiver_base_cf_sptr> rx;     /*!< receiver. */
    gr::blocks::add_ff::sptr           add0;   /* Audio downmix */
    gr::blocks::add_ff::sptr           add1;   /* Audio downmix */
    gr::blocks::multiply_const_ff::sptr mc0;   /* Audio downmix */
    gr::blocks::multiply_const_ff::sptr mc1;   /* Audio downmix */
    gr::blocks::null_source::sptr null_src;    /* temporary workaround */

    dc_corr_cc_sptr           dc_corr;   /*!< DC corrector block. */
    iq_swap_cc_sptr           iq_swap;   /*!< I/Q swapping block. */

    rx_fft_c_sptr             iq_fft;     /*!< Baseband FFT block. */
    rx_fft_f_sptr             audio_fft;  /*!< Audio FFT block. */

    gr::blocks::file_sink::sptr         iq_sink;     /*!< I/Q file sink. */

    gr::blocks::wavfile_source::sptr    wav_src;    /*!< WAV file source for playback. */
    gr::blocks::null_sink::sptr         audio_null_sink0; /*!< Audio null sink used during playback. */
    gr::blocks::null_sink::sptr         audio_null_sink1; /*!< Audio null sink used during playback. */

    sniffer_f_sptr    sniffer;    /*!< Sample sniffer for data decoders. */
    resampler_ff_sptr sniffer_rr; /*!< Sniffer resampler. */

#ifdef WITH_PULSEAUDIO
    pa_sink_sptr              audio_snk;  /*!< Pulse audio sink. */
#elif WITH_PORTAUDIO
    portaudio_sink_sptr       audio_snk;  /*!< portaudio sink */
#else
    gr::audio::sink::sptr     audio_snk;  /*!< gr audio sink */
#endif
    audio_rec_event_handler_t d_audio_rec_event_handler;
    //! Get a path to a file containing random bytes
    static std::string get_zero_file(void);
    static void audio_rec_event(receiver * self, int idx, std::string filename,
                                bool is_running);
};

#endif // RECEIVER_H
