/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#include <cmath>
#include <iostream>
#include <unistd.h>

#include <gnuradio/prefs.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <osmosdr/source.h>
#include <osmosdr/ranges.h>

#include "applications/gqrx/receiver.h"
#include "dsp/correct_iq_cc.h"
#include "dsp/rx_fft.h"
#include "receivers/nbrx.h"
#include "receivers/wfmrx.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif


/*! \brief Public contructor.
 *  \param input_device Input device specifier.
 *  \param audio_device Audio output device specifier,
 *                      e.g. hw:0 when using ALSA or Portaudio.
 *
 * \todo Option to use UHD device instead of FCD.
 */
receiver::receiver(const std::string input_device, const std::string audio_device)
    : d_running(false),
      d_input_rate(96000.0),
      d_audio_rate(48000),
      d_rf_freq(144800000.0),
      d_filter_offset(0.0),
      d_recording_iq(false),
      d_recording_wav(false),
      d_sniffer_active(false),
      d_iq_rev(false),
      d_dc_cancel(false),
      d_iq_balance(false),
      d_demod(RX_DEMOD_OFF)
{

    tb = gr::make_top_block("gqrx");

    if (input_device.empty())
    {
        // FIXME: other OS
        src = osmosdr::source::make("file=/dev/random,freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    else
    {
        input_devstr = input_device;
        src = osmosdr::source::make(input_device);
    }

    // create I/Q sink and close it
    iq_sink = gr::blocks::file_sink::make(sizeof(gr_complex), "/dev/null", false);
    iq_sink->set_unbuffered(true);
    iq_sink->close();

    rx = make_nbrx(d_input_rate, d_audio_rate);
    lo = gr::analog::sig_source_c::make(d_input_rate, gr::analog::GR_SIN_WAVE, 0.0, 1.0);
    mixer = gr::blocks::multiply_cc::make();

    iq_swap = make_iq_swap_cc(false);
    dc_corr = make_dc_corr_cc(d_input_rate, 1.0);
    iq_fft = make_rx_fft_c(4096u, 0);

    audio_fft = make_rx_fft_f(4096u);
    audio_gain0 = gr::blocks::multiply_const_ff::make(0.1);
    audio_gain1 = gr::blocks::multiply_const_ff::make(0.1);

    wav_sink = gr::blocks::wavfile_sink::make("/dev/null", 2,
                                              (unsigned int) d_audio_rate,
                                              16);

    audio_udp_sink = make_udp_sink_f();

#ifdef WITH_PULSEAUDIO
    audio_snk = make_pa_sink(audio_device, d_audio_rate, "GQRX", "Audio output");
#else
    audio_snk = gr::audio::sink::make(d_audio_rate, audio_device, true);
#endif

    output_devstr = audio_device;

    /* wav sink and source is created when rec/play is started */
    audio_null_sink0 = gr::blocks::null_sink::make(sizeof(float));
    audio_null_sink1 = gr::blocks::null_sink::make(sizeof(float));
    sniffer = make_sniffer_f();
    /* sniffer_rr is created at each activation. */

    set_demod(RX_DEMOD_NFM);

#ifndef QT_NO_DEBUG_OUTPUT
    gr::prefs pref;
    std::cout << "Using audio backend: "
              << pref.get_string("audio", "audio_module", "N/A")
              << std::endl;
#endif
}


/*! \brief Public destructor. */
receiver::~receiver()
{
    tb->stop();

    /* FIXME: delete blocks? */
}


/*! \brief Start the receiver. */
void receiver::start()
{
    if (!d_running)
    {
        tb->start();
        d_running = true;
    }
}

/*! \brief Stop the receiver. */
void receiver::stop()
{
    if (d_running)
    {
        tb->stop();
        tb->wait(); // If the graph is needed to run again, wait() must be called after stop
        d_running = false;
    }
}

/*! \brief Select new input device.
 *
 * \bug When using ALSA, program will crash if the new device
 *      is the same as the previously used device:
 *      audio_alsa_source[hw:1]: Device or resource busy
 */
void receiver::set_input_device(const std::string device)
{
    if (device.empty())
        return;

    if (input_devstr.compare(device) == 0)
    {
#ifndef QT_NO_DEBUG_OUTPUT
        std::cout << "No change in input device:" << std::endl
                  << "  old: " << input_devstr << std::endl
                  << "  new: " << device << std::endl;
#endif
        return;
    }

    input_devstr = device;

    // tb->lock() can hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    tb->disconnect(src, 0, iq_swap, 0);
    src.reset();
    src = osmosdr::source::make(device);

    tb->connect(src, 0, iq_swap, 0);

    if (d_running)
        tb->start();
}


/*! \brief Select new audio output device. */
void receiver::set_output_device(const std::string device)
{
    if (output_devstr.compare(device) == 0)
    {
#ifndef QT_NO_DEBUG_OUTPUT
        std::cout << "No change in output device:" << std::endl
                  << "  old: " << output_devstr << std::endl
                  << "  new: " << device << std::endl;
#endif
        return;
    }

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "New audio output device:" << std::endl
              << "   old: " << output_devstr << std::endl
              << "   new: " << device << std::endl;
#endif

    output_devstr = device;

    tb->lock();

    tb->disconnect(audio_gain0, 0, audio_snk, 0);
    tb->disconnect(audio_gain1, 0, audio_snk, 1);
    audio_snk.reset();

#ifdef WITH_PULSEAUDIO
    audio_snk = make_pa_sink(device, d_audio_rate);
#else
    audio_snk = gr::audio::sink::make(d_audio_rate, device, true);
#endif

    tb->connect(audio_gain0, 0, audio_snk, 0);
    tb->connect(audio_gain1, 0, audio_snk, 1);

    tb->unlock();
}

/*! \brief Get a list of available antenna connectors. */
std::vector<std::string> receiver::get_antennas(void)
{
    return src->get_antennas();
}

/*! \brief Select antenna conenctor. */
void receiver::set_antenna(const std::string &antenna)
{
    src->set_antenna(antenna);
}

/*! \brief Set new input sample rate.
 *  \param rate The desired input rate
 *  \return The actual sample rate set or 0 if there was an error with the
 *          device.
 */
double receiver::set_input_rate(double rate)
{
    tb->lock();
    d_input_rate = src->set_sample_rate(rate);

    if (d_input_rate == 0)
    {
        // This can be the case when no device is attached and gr-osmosdr
        // puts in a null_source with rate 100 ksps.
        // ...
        // Or with some over the top fucked up rtl dongles that appear to work
        // but return an error here. So we just ignore it.
        std::cerr << std::endl;
        std::cerr << "Failed to set RX input rate to " << rate << std::endl;
        std::cerr << "Your device may not be working properly." << std::endl;
        std::cerr << std::endl;
        d_input_rate = rate;
    }

    dc_corr->set_sample_rate(d_input_rate);
    rx->set_quad_rate(d_input_rate);
    lo->set_sampling_freq(d_input_rate);
    tb->unlock();

    return d_input_rate;
}


/*! \brief Get current input sample rate. */
double receiver::get_input_rate()
{
    return d_input_rate;
}

/*! \brief Set new analog bandwidth.
 *  \param bw The new bandwidth.
 *  \return The actual bandwidth.
 */
double receiver::set_analog_bandwidth(double bw)
{
    return src->set_bandwidth(bw);
}

/*! \brief Get current analog bandwidth. */
double receiver::get_analog_bandwidth()
{
    return src->get_bandwidth();
}



/*! \brief Set I/Q reversed. */
void receiver::set_iq_swap(bool reversed)
{
    if (reversed == d_iq_rev)
        return;

    d_iq_rev = reversed;
    iq_swap->set_enabled(d_iq_rev);
}

/*! \brief Get current I/Q reversed setting.
 *  \retval true I/Q swappign is enabled.
 *  \retval false I/Q swapping is disabled.
 */
bool receiver::get_iq_swap(void)
{
    return d_iq_rev;
}

/*! \brief Enable/disable automatic DC removal in the I/Q stream.
 *  \param enable Whether DC removal should enabled or not.
 */
void receiver::set_dc_cancel(bool enable)
{
    if (enable == d_dc_cancel)
        return;

    d_dc_cancel = enable;

    // until we have a way to switch on/off
    // inside the dc_corr_cc we do a reconf
    rx_demod demod = d_demod;
    d_demod = RX_DEMOD_OFF;
    set_demod(demod);
}

/*! \brief Get auto DC cancel status.
 *  \retval true  Automatic DC removal is enabled.
 *  \retval false Automatic DC removal is disabled.
 */
bool receiver::get_dc_cancel(void)
{
    return d_dc_cancel;
}

/*! \brief Enable/disable automatic I/Q balance.
 *  \param enable Whether automatic I/Q balance should be enabled.
 */
void receiver::set_iq_balance(bool enable)
{
    if (enable == d_iq_balance)
        return;

    d_iq_balance = enable;

    src->set_iq_balance_mode(enable ? 2 : 0);
}

/*! \brief Get auto I/Q balance status.
 *  \retval true  Automatic I/Q balance is enabled.
 *  \retval false Automatic I/Q balance is disabled.
 */
bool receiver::get_iq_balance(void)
{
    return d_iq_balance;
}
/*! \brief Set RF frequency.
 *  \param freq_hz The desired frequency in Hz.
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
 *  \sa get_rf_freq()
 */
receiver::status receiver::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    src->set_center_freq(d_rf_freq);
    // FIXME: read back frequency?

    return STATUS_OK;
}

/*! \brief Get RF frequency.
 *  \return The current RF frequency.
 *  \sa set_rf_freq()
 */
double receiver::get_rf_freq()
{
    d_rf_freq = src->get_center_freq();

    return d_rf_freq;
}

/*! \brief Get the RF frequency range of the current input device.
 *  \param start The lower limit of the range in Hz.
 *  \param stop  The upper limit of the range in Hz.
 *  \param step  The frequency step in Hz.
 *  \returns STATUS_OK if the range could be retrieved, STATUS_ERROR if an error has occurred.
 */
receiver::status receiver::get_rf_range(double *start, double *stop, double *step)
{
    osmosdr::freq_range_t range;

    range = src->get_freq_range();

    // currently range is empty for all but E4000
    if (!range.empty())
    {
        if (range.start() < range.stop())
        {
            *start = range.start();
            *stop  = range.stop();
            *step  = range.step();  /** FIXME: got 0 for rtl-sdr? **/

            return STATUS_OK;
        }
    }

    return STATUS_ERROR;
}

/*! \brief Get the names of available gain stages. */
std::vector<std::string> receiver::get_gain_names()
{
    return src->get_gain_names();
}

/*! \brief Get gain range for a specific stage.
 *  \param[in]  name The name of the gain stage.
 *  \param[out] start Lower limit for this gain setting.
 *  \param[out] stop  Upper limit for this gain setting.
 *  \param[out] step  The resolution for this gain setting.
 *
 * This function retunrs the range for the requested gain stage.
 */
receiver::status receiver::get_gain_range(std::string &name, double *start, double *stop, double *step)
{
    osmosdr::gain_range_t range;

    range = src->get_gain_range(name);
    *start = range.start();
    *stop  = range.stop();
    *step  = range.step();

    return STATUS_OK;
}

receiver::status receiver::set_gain(std::string name, double value)
{
    src->set_gain(value, name);

    return STATUS_OK;
}

double receiver::get_gain(std::string name)
{
    return src->get_gain(name);
}


/*! \brief Set RF gain.
 *  \param gain_rel The desired relative gain between 0.0 and 1.0 (use -1 for AGC where supported).
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
 */
receiver::status receiver::set_auto_gain(bool automatic)
{
    src->set_gain_mode(automatic);

    return STATUS_OK;
}


/*! \brief Set filter offset.
 *  \param offset_hz The desired filter offset in Hz.
 *  \return RX_STATUS_ERROR if the tuning offset is out of range.
 *
 * This method sets a new tuning offset for the receiver. The tuning offset is used
 * to tune within the passband, i.e. select a specific channel within the received
 * spectrum.
 *
 * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
 * logical limit.
 *
 * \sa get_filter_offset()
 */
receiver::status receiver::set_filter_offset(double offset_hz)
{
    d_filter_offset = offset_hz;
    lo->set_frequency(-d_filter_offset);

    return STATUS_OK;
}

/*! \brief Get filterm offset.
 *  \return The current filter offset.
 *  \sa set_filter_offset()
 */
double receiver::get_filter_offset()
{
    return d_filter_offset;
}


receiver::status receiver::set_filter(double low, double high, filter_shape shape)
{
    double trans_width;

    if ((low >= high) || (abs(high-low) < RX_FILTER_MIN_WIDTH))
        return STATUS_ERROR;

    switch (shape) {

    case FILTER_SHAPE_SOFT:
        trans_width = abs(high - low) * 0.4;
        break;

    case FILTER_SHAPE_SHARP:
        trans_width = abs(high - low) * 0.05;
        break;

    case FILTER_SHAPE_NORMAL:
    default:
        trans_width = abs(high - low) * 0.2;
        break;

    }

    rx->set_filter(low, high, trans_width);

    return STATUS_OK;
}

/**
receiver::status receiver::set_filter_low(double freq_hz)
{
    return STATUS_OK;
}


receiver::status receiver::set_filter_high(double freq_hz)
{
    return STATUS_OK;
}


receiver::status receiver::set_filter_shape(filter_shape shape)
{
    return STATUS_OK;
}
**/

receiver::status receiver::set_freq_corr(double ppm)
{
    src->set_freq_corr(ppm);

    return STATUS_OK;
}

/*! \brief Get current signal power.
 *  \param dbfs Whether to use dbfs or absolute power.
 *  \return The current signal power.
 *
 * This method returns the current signal power detected by the receiver. The detector
 * is located after the band pass filter. The full scale is 1.0
 */
float receiver::get_signal_pwr(bool dbfs)
{
    return rx->get_signal_level(dbfs);
}

/*! \brief Set new FFT size. */
void receiver::set_iq_fft_size(int newsize)
{
    iq_fft->set_fft_size(newsize);
}

/*! \brief Get latest baseband FFT data. */
void receiver::get_iq_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize)
{
    iq_fft->get_fft_data(fftPoints, fftsize);
}

/*! \brief Get latest audio FFT data. */
void receiver::get_audio_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize)
{
    audio_fft->get_fft_data(fftPoints, fftsize);
}

receiver::status receiver::set_nb_on(int nbid, bool on)
{
    if (rx->has_nb())
        rx->set_nb_on(nbid, on);

    return STATUS_OK; // FIXME
}

receiver::status receiver::set_nb_threshold(int nbid, float threshold)
{
    if (rx->has_nb())
        rx->set_nb_threshold(nbid, threshold);

    return STATUS_OK; // FIXME
}


/*! \brief Set squelch level.
 *  \param level_db The new level in dBFS.
 */
receiver::status receiver::set_sql_level(double level_db)
{
    if (rx->has_sql())
        rx->set_sql_level(level_db);

    return STATUS_OK; // FIXME
}


/*! \brief Set squelch alpha */
receiver::status receiver::set_sql_alpha(double alpha)
{
    if (rx->has_sql())
        rx->set_sql_alpha(alpha);

    return STATUS_OK; // FIXME
}

/*! \brief Enable/disable receiver AGC.
 *
 * When AGC is disabled a fixed manual gain is used, see set_agc_manual_gain().
 */
receiver::status receiver::set_agc_on(bool agc_on)
{
    if (rx->has_agc())
        rx->set_agc_on(agc_on);

    return STATUS_OK; // FIXME
}

/*! \brief Enable/disable AGC hang. */
receiver::status receiver::set_agc_hang(bool use_hang)
{
    if (rx->has_agc())
        rx->set_agc_hang(use_hang);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC threshold. */
receiver::status receiver::set_agc_threshold(int threshold)
{
    if (rx->has_agc())
        rx->set_agc_threshold(threshold);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC slope. */
receiver::status receiver::set_agc_slope(int slope)
{
    if (rx->has_agc())
        rx->set_agc_slope(slope);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC decay time. */
receiver::status receiver::set_agc_decay(int decay_ms)
{
    if (rx->has_agc())
        rx->set_agc_decay(decay_ms);

    return STATUS_OK; // FIXME
}

/*! \brief Set fixed gain used when AGC is OFF. */
receiver::status receiver::set_agc_manual_gain(int gain)
{
    if (rx->has_agc())
        rx->set_agc_manual_gain(gain);

    return STATUS_OK; // FIXME
}

receiver::status receiver::set_demod(rx_demod demod)
{
    status ret = STATUS_OK;

    // Allow reconf using same demod to provide a workaround
    // for the "jerky streaming" we may experience with rtl
    // dongles (the jerkyness disappears when we run this function)
    //if (demod == d_demod)
    //    return ret;

    // tb->lock() seems to hang occasioanlly
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    tb->disconnect_all();

    switch (demod)
    {
    case RX_DEMOD_OFF:
        connect_all(RX_CHAIN_NONE);
        break;

    case RX_DEMOD_NONE:
        connect_all(RX_CHAIN_NBRX);
        rx->set_demod(nbrx::NBRX_DEMOD_NONE);
        break;

    case RX_DEMOD_AM:
        connect_all(RX_CHAIN_NBRX);
        rx->set_demod(nbrx::NBRX_DEMOD_AM);
        break;

    case RX_DEMOD_NFM:
        connect_all(RX_CHAIN_NBRX);
        rx->set_demod(nbrx::NBRX_DEMOD_FM);
        break;

    case RX_DEMOD_WFM_M:
        connect_all(RX_CHAIN_WFMRX);
        rx->set_demod(wfmrx::WFMRX_DEMOD_MONO);
        break;

    case RX_DEMOD_WFM_S:
        connect_all(RX_CHAIN_WFMRX);
        rx->set_demod(wfmrx::WFMRX_DEMOD_STEREO);
        break;

    case RX_DEMOD_SSB:
        connect_all(RX_CHAIN_NBRX);
        rx->set_demod(nbrx::NBRX_DEMOD_SSB);
        break;

    default:
        ret = STATUS_ERROR;
        break;
    }

    d_demod = demod;

    if (d_running)
        tb->start();

    return ret;
}

/*! \brief Set maximum deviation of the FM demodulator.
 *  \param maxdev_hz The new maximum deviation in Hz.
 */
receiver::status receiver::set_fm_maxdev(float maxdev_hz)
{
    if (rx->has_fm())
        rx->set_fm_maxdev(maxdev_hz);

    return STATUS_OK;
}

receiver::status receiver::set_fm_deemph(double tau)
{
    if (rx->has_fm())
        rx->set_fm_deemph(tau);

    return STATUS_OK;
}

receiver::status receiver::set_am_dcr(bool enabled)
{
    if (rx->has_am())
        rx->set_am_dcr(enabled);

    return STATUS_OK;
}

receiver::status receiver::set_af_gain(float gain_db)
{
    float k;

    /* convert dB to factor */
    k = pow(10.0, gain_db / 20.0);
    //std::cout << "G:" << gain_db << "dB / K:" << k << std::endl;
    audio_gain0->set_k(k);
    audio_gain1->set_k(k);

    return STATUS_OK;
}


/*! \brief Start WAV file recorder.
 *  \param filename The filename where to record.
 *
 * A new recorder object is created every time we start recording and deleted every time
 * we stop recording. The idea of creating one object and starting/stopping using different
 * file names does not work with WAV files (the initial /tmp/gqrx.wav will not be stopped
 * because the wav file can not be empty). See https://github.com/csete/gqrx/issues/36
 */
receiver::status receiver::start_audio_recording(const std::string filename)
{
    if (d_recording_wav)
    {
        /* error - we are already recording */
        std::cout << "ERROR: Can not start audio recorder (already recording)" << std::endl;

        return STATUS_ERROR;
    }
    if (!d_running)
    {
        /* receiver is not running */
        std::cout << "Can not start audio recorder (receiver not running)" << std::endl;

        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();

    // if this fails, we don't want to go and crash now, do we
    try {
        wav_sink->open(filename.c_str());
        wav_sink->set_sample_rate((unsigned int) d_audio_rate);
    }
    catch (std::runtime_error &e) {
        std::cout << "Error opening " << filename << ": " << e.what() << std::endl;
        return STATUS_ERROR;
    }

    tb->connect(rx, 0, wav_sink, 0);
    tb->connect(rx, 1, wav_sink, 1);
    tb->unlock();
    d_recording_wav = true;

    std::cout << "Recording audio to " << filename << std::endl;

    return STATUS_OK;
}


/*! \brief Stop WAV file recorder. */
receiver::status receiver::stop_audio_recording()
{
    if (!d_recording_wav) {
        /* error: we are not recording */
        std::cout << "ERROR: Can not stop audio recorder (not recording)" << std::endl;

        return STATUS_ERROR;
    }
    if (!d_running)
    {
        /* receiver is not running */
        std::cout << "Can not stop audio recorder (receiver not running)" << std::endl;

        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();
    wav_sink->close();
    tb->disconnect(rx, 0, wav_sink, 0);
    tb->disconnect(rx, 1, wav_sink, 1);
    tb->unlock();
    d_recording_wav = false;

    std::cout << "Audio recorder stopped" << std::endl;

    return STATUS_OK;
}


/*! \brief Start audio playback. */
receiver::status receiver::start_audio_playback(const std::string filename)
{
    if (!d_running)
    {
        /* receiver is not running */
        std::cout << "Can not start audio playback (receiver not running)" << std::endl;

        return STATUS_ERROR;
    }

    try {
        // output ports set automatically from file
        wav_src = gr::blocks::wavfile_source::make(filename.c_str(), false);
    }
    catch (std::runtime_error &e) {
        std::cout << "Error loading " << filename << ": " << e.what() << std::endl;
        return STATUS_ERROR;
    }

    /** FIXME: We can only handle native rate (should maybe use the audio_rr)? */
    unsigned int audio_rate = (unsigned int) d_audio_rate;
    if (wav_src->sample_rate() != audio_rate)
    {
        std::cout << "BUG: Can not handle sample rate " << wav_src->sample_rate() << std::endl;
        wav_src.reset();

        return STATUS_ERROR;
    }

    /** FIXME: We can only handle stereo files */
    if (wav_src->channels() != 2)
    {
        std::cout << "BUG: Can not handle other than 2 channels. File has " << wav_src->channels() << std::endl;
        wav_src.reset();

        return STATUS_ERROR;
    }

    stop();
    /* route demodulator output to null sink */
    tb->disconnect(rx, 0, audio_gain0, 0);
    tb->disconnect(rx, 1, audio_gain1, 0);
    tb->disconnect(rx, 0, audio_fft, 0);
    tb->disconnect(rx, 0, audio_udp_sink, 0);
    tb->connect(rx, 0, audio_null_sink0, 0); /** FIXME: other channel? */
    tb->connect(rx, 1, audio_null_sink1, 0); /** FIXME: other channel? */
    tb->connect(wav_src, 0, audio_gain0, 0);
    tb->connect(wav_src, 1, audio_gain1, 0);
    tb->connect(wav_src, 0, audio_fft, 0);
    tb->connect(wav_src, 0, audio_udp_sink, 0);
    start();

    std::cout << "Playing audio from " << filename << std::endl;

    return STATUS_OK;
}


/*! \brief Stop audio playback. */
receiver::status receiver::stop_audio_playback()
{
    /* disconnect wav source and reconnect receiver */
    stop();
    tb->disconnect(wav_src, 0, audio_gain0, 0);
    tb->disconnect(wav_src, 1, audio_gain1, 0);
    tb->disconnect(wav_src, 0, audio_fft, 0);
    tb->disconnect(wav_src, 0, audio_udp_sink, 0);
    tb->disconnect(rx, 0, audio_null_sink0, 0);
    tb->disconnect(rx, 1, audio_null_sink1, 0);
    tb->connect(rx, 0, audio_gain0, 0);
    tb->connect(rx, 1, audio_gain1, 0);
    tb->connect(rx, 0, audio_fft, 0);  /** FIXME: other channel? */
    tb->connect(rx, 0, audio_udp_sink, 0);
    start();

    /* delete wav_src since we can not change file name */
    wav_src.reset();

    return STATUS_OK;
}


/*! \brief Start UDP streaming of audio. */
receiver::status receiver::start_udp_streaming(const std::string host, int port)
{
    audio_udp_sink->start_streaming(host, port);
    return STATUS_OK;
}

/*! \brief Stop UDP streaming of audio. */
receiver::status receiver::stop_udp_streaming()
{
    audio_udp_sink->stop_streaming();
    return STATUS_OK;
}


/*! \brief Start I/Q data recorder.
 *  \param filename The filename where to record.
 */
receiver::status receiver::start_iq_recording(const std::string filename)
{
    receiver::status status = STATUS_OK;

    if (d_recording_iq) {
        std::cout << __func__ << ": already recording" << std::endl;
        return STATUS_ERROR;
    }

    // iq_sink was created in the constructor
    if (iq_sink) {
        tb->lock();
        if (!iq_sink->open(filename.c_str()))
        {
            status = STATUS_ERROR;
        }
        else
        {
            tb->connect(src, 0, iq_sink, 0);
            d_recording_iq = true;
        }
        tb->unlock();
    }
    else {
        std::cout << __func__ << ": I/Q file sink does not exist" << std::endl;
        return STATUS_ERROR;
    }

    return status;
}


/*! \brief Stop I/Q data recorder. */
receiver::status receiver::stop_iq_recording()
{
    if (!d_recording_iq) {
        /* error: we are not recording */
        return STATUS_ERROR;
    }

    tb->lock();
    iq_sink->close();
    tb->disconnect(src, 0, iq_sink, 0);
    tb->unlock();
    d_recording_iq = false;

    return STATUS_OK;
}

/*! \brief Seek to position in IQ file source.
 *  \param pos Byte offset from the beginning of the file.
 */
receiver::status receiver::seek_iq_file(long pos)
{
    receiver::status status = STATUS_OK;

    tb->lock();

    if (src->seek(pos, SEEK_SET))
    {
        status = STATUS_OK;
    }
    else
    {
        status = STATUS_ERROR;
    }

    tb->unlock();

    return status;
}

/*! \brief Start data sniffer.
 *  \param buffsize The buffer that should be used in the sniffer.
 *  \return STATUS_OK if the sniffer was started, STATUS_ERROR if the sniffer is already in use.
 */
receiver::status receiver::start_sniffer(unsigned int samprate, int buffsize)
{
    if (d_sniffer_active) {
        /* sniffer already in use */
        return STATUS_ERROR;
    }

    sniffer->set_buffer_size(buffsize);
    sniffer_rr = make_resampler_ff((float)samprate/(float)d_audio_rate);
    tb->lock();
    tb->connect(rx, 0, sniffer_rr, 0);
    tb->connect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = true;

    return STATUS_OK;
}

/*! \brief Stop data sniffer.
 *  \return STATUS_ERROR i the sniffer is not currently active.
 */
receiver::status receiver::stop_sniffer()
{
    if (!d_sniffer_active) {
        return STATUS_ERROR;
    }

    tb->lock();
    tb->disconnect(rx, 0, sniffer_rr, 0);
    tb->disconnect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = false;

    /* delete resampler */
    sniffer_rr.reset();

    return STATUS_OK;
}

/*! \brief Get sniffer data. */
void receiver::get_sniffer_data(float * outbuff, unsigned int &num)
{
    sniffer->get_samples(outbuff, num);
}

/*! \brief Convenience function to connect all blocks. */
void receiver::connect_all(rx_chain type)
{
    switch (type)
    {
    case RX_CHAIN_NONE:
        tb->connect(src, 0, iq_swap, 0);
        if (d_dc_cancel)
        {
            tb->connect(iq_swap, 0, dc_corr, 0);
            tb->connect(dc_corr, 0, iq_fft, 0);
        }
        else
        {
            tb->connect(iq_swap, 0, iq_fft, 0);
        }
        break;

    case RX_CHAIN_NBRX:
        if (rx->name() != "NBRX")
        {
            rx.reset();
            rx = make_nbrx(d_input_rate, d_audio_rate);
        }
        tb->connect(src, 0, iq_swap, 0);
        if (d_dc_cancel)
        {
            tb->connect(iq_swap, 0, dc_corr, 0);
            tb->connect(dc_corr, 0, iq_fft, 0);
            tb->connect(dc_corr, 0, mixer, 0);
        }
        else
        {
            tb->connect(iq_swap, 0, iq_fft, 0);
            tb->connect(iq_swap, 0, mixer, 0);
        }
        tb->connect(lo, 0, mixer, 1);
        tb->connect(mixer, 0, rx, 0);
        tb->connect(rx, 0, audio_fft, 0);
        tb->connect(rx, 0, audio_udp_sink, 0);
        tb->connect(rx, 0, audio_gain0, 0);
        tb->connect(rx, 1, audio_gain1, 0);
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);
        break;

    case RX_CHAIN_WFMRX:
        if (rx->name() != "WFMRX")
        {
            rx.reset();
            rx = make_wfmrx(d_input_rate, d_audio_rate);
        }
        tb->connect(src, 0, iq_swap, 0);
        if (d_dc_cancel)
        {
            tb->connect(iq_swap, 0, dc_corr, 0);
            tb->connect(dc_corr, 0, iq_fft, 0);
            tb->connect(dc_corr, 0, mixer, 0);
        }
        else
        {
            tb->connect(iq_swap, 0, iq_fft, 0);
            tb->connect(iq_swap, 0, mixer, 0);
        }
        tb->connect(lo, 0, mixer, 1);
        tb->connect(mixer, 0, rx, 0);
        tb->connect(rx, 0, audio_fft, 0);
        tb->connect(rx, 0, audio_udp_sink, 0);
        tb->connect(rx, 0, audio_gain0, 0);
        tb->connect(rx, 1, audio_gain1, 0);
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);
        break;

    default:
        break;
    }

    // reconnect recorders and sniffers
    if (d_recording_iq)
    {
        tb->connect(src, 0, iq_sink, 0);
    }

    if (d_recording_wav)
    {
        tb->connect(rx, 0, wav_sink, 0);
        tb->connect(rx, 1, wav_sink, 1);
    }

    if (d_sniffer_active)
    {
        tb->connect(rx, 0, sniffer_rr, 0);
        tb->connect(sniffer_rr, 0, sniffer, 0);
    }
}

void receiver::get_rds_data(std::string &outbuff, int &num)
{
    rx->get_rds_data(outbuff, num);
}

void receiver::start_rds_decoder()
{
    stop();
    rx->start_rds_decoder();
    start();
}

void receiver::stop_rds_decoder()
{
    stop();
    rx->stop_rds_decoder();
    start();
}

bool receiver::is_rds_decoder_active()
{
    return rx->is_rds_decoder_active();
}

void receiver::reset_rds_parser()
{
    rx->reset_rds_parser();
}
