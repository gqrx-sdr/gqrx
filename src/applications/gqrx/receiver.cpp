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
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <QDebug>

#include <gnuradio/prefs.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>
#include <osmosdr/ranges.h>

#include "applications/gqrx/receiver.h"
#include "dsp/correct_iq_cc.h"
#include "dsp/filter/fir_decim.h"
#include "dsp/rx_fft.h"
#include "receivers/nbrx.h"
#include "receivers/wfmrx.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif


/**
 * @brief Public constructor.
 * @param input_device Input device specifier.
 * @param audio_device Audio output device specifier,
 *                     e.g. hw:0 when using ALSA or Portaudio.
 */
receiver::receiver(const std::string input_device,
                   const std::string audio_device,
                   unsigned int decimation)
    : d_current(-1),
      d_active(0),
      d_running(false),
      d_input_rate(96000.0),
      d_audio_rate(48000),
      d_decim(decimation),
      d_rf_freq(144800000.0),
      d_recording_iq(false),
      d_sniffer_active(false),
      d_iq_rev(false),
      d_dc_cancel(false),
      d_iq_balance(false),
      d_mute(false)
{

    tb = gr::make_top_block("gqrx");

    if (input_device.empty())
    {
        src = osmosdr::source::make("file="+escape_filename(get_zero_file())+",freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    else
    {
        input_devstr = input_device;
        src = osmosdr::source::make(input_device);
    }

    // input decimator
    if (d_decim >= 2)
    {
        try
        {
            input_decim = make_fir_decim_cc(d_decim);
        }
        catch (std::range_error &e)
        {
            std::cout << "Error creating input decimator " << d_decim
                      << ": " << e.what() << std::endl
                      << "Using decimation 1." << std::endl;
            d_decim = 1;
        }

        d_decim_rate = d_input_rate / (double)d_decim;
    }
    else
    {
        d_decim_rate = d_input_rate;
    }

    rx.reserve(RX_MAX);
    rx.clear();
    d_current = 0;
    rx.push_back(make_nbrx(d_decim_rate, d_audio_rate));
    rx[d_current]->set_index(d_current);
    rx[d_current]->set_rec_event_handler(std::bind(audio_rec_event, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3));

    iq_swap = make_iq_swap_cc(false);
    iq_src = iq_swap;
    dc_corr = make_dc_corr_cc(d_decim_rate, 1.0);
    iq_fft = make_rx_fft_c(DEFAULT_FFT_SIZE, d_decim_rate, gr::fft::window::WIN_HANN);

    audio_fft = make_rx_fft_f(DEFAULT_FFT_SIZE, d_audio_rate, gr::fft::window::WIN_HANN);

    add0 = gr::blocks::add_ff::make(1);
    add1 = gr::blocks::add_ff::make(1);
    mc0  = gr::blocks::multiply_const_ff::make(1.0, 1);
    mc1  = gr::blocks::multiply_const_ff::make(1.0, 1);
    null_src = gr::blocks::null_source::make(sizeof(float));

#ifdef WITH_PULSEAUDIO
    audio_snk = make_pa_sink(audio_device, d_audio_rate, "GQRX", "Audio output");
#elif WITH_PORTAUDIO
    audio_snk = make_portaudio_sink(audio_device, d_audio_rate, "GQRX", "Audio output");
#else
    audio_snk = gr::audio::sink::make(d_audio_rate, audio_device, true);
#endif

    output_devstr = audio_device;

    /* wav sink and source is created when rec/play is started */
    audio_null_sink0 = gr::blocks::null_sink::make(sizeof(float));
    audio_null_sink1 = gr::blocks::null_sink::make(sizeof(float));
    sniffer = make_sniffer_f();
    /* sniffer_rr is created at each activation. */

    set_demod(Modulations::MODE_OFF);
    reconnect_all();

    gr::prefs pref;
    qDebug() << "Using audio backend:"
             << pref.get_string("audio", "audio_module", "N/A").c_str();

}

receiver::~receiver()
{
    tb->stop();
}


/** Start the receiver. */
void receiver::start()
{
    if (!d_running)
    {
        tb->start();
        d_running = true;
    }
}

/** Stop the receiver. */
void receiver::stop()
{
    if (d_running)
    {
        tb->stop();
        tb->wait(); // If the graph is needed to run again, wait() must be called after stop
        d_running = false;
    }
}

/**
 * @brief Select new input device.
 * @param device
 */
void receiver::set_input_device(const std::string device)
{
    qDebug() << "Set input device:";
    qDebug() << "  old:" << input_devstr.c_str();
    qDebug() << "  new:" << device.c_str();

    std::string error = "";

    if (device.empty())
        return;

    input_devstr = device;

    // tb->lock() can hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    tb->disconnect_all();
    for (auto& rxc : rx)
        rxc->connected(false);

#if GNURADIO_VERSION < 0x030802
    //Work around GNU Radio bug #3184
    //temporarily connect dummy source to ensure that previous device is closed
    src = osmosdr::source::make("file="+escape_filename(get_zero_file())+",freq=428e6,rate=96000,repeat=true,throttle=true");
    tb->connect(src, 0, iq_swap, 0);
    tb->connect(iq_swap, 0, iq_fft, 0);
    tb->start();
    tb->stop();
    tb->wait();
    tb->disconnect_all();
#else
    src.reset();
#endif

    try
    {
        src = osmosdr::source::make(device);
    }
    catch (std::exception &x)
    {
        error = x.what();
        src = osmosdr::source::make("file="+escape_filename(get_zero_file())+",freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    reconnect_all(true);
    if (src->get_sample_rate() != 0)
        set_input_rate(src->get_sample_rate());

    if (d_running)
        tb->start();

    if (error != "")
    {
        throw std::runtime_error(error);
    }
}


/**
 * @brief Select new audio output device.
 * @param device
 */
void receiver::set_output_device(const std::string device)
{
    qDebug() << "Set output device:";
    qDebug() << "   old:" << output_devstr.c_str();
    qDebug() << "   new:" << device.c_str();

    output_devstr = device;

    tb->lock();

    if (d_active > 0)
    {
        try
        {
            tb->disconnect(mc0, 0, audio_snk, 0);
            tb->disconnect(mc1, 1, audio_snk, 1);
        }
        catch(std::exception &x)
        {
        }
    }
    audio_snk.reset();

    try {
#ifdef WITH_PULSEAUDIO
        audio_snk = make_pa_sink(device, d_audio_rate, "GQRX", "Audio output");
#elif WITH_PORTAUDIO
        audio_snk = make_portaudio_sink(device, d_audio_rate, "GQRX", "Audio output");
#else
        audio_snk = gr::audio::sink::make(d_audio_rate, device, true);
#endif

        if (d_active > 0)
        {
            tb->connect(mc0, 0, audio_snk, 0);
            tb->connect(mc1, 0, audio_snk, 1);
        }

        tb->unlock();

    } catch (std::exception &x) {
        tb->unlock();
        // handle problems on non-freeing devices
        throw x;
    }
}

/** Get a list of available antenna connectors. */
std::vector<std::string> receiver::get_antennas(void) const
{
    return src->get_antennas();
}

/** Select antenna connector. */
void receiver::set_antenna(const std::string &antenna)
{
    if (!antenna.empty())
    {
        src->set_antenna(antenna);
    }
}

/**
 * @brief Set new input sample rate.
 * @param rate The desired input rate
 * @return The actual sample rate set or 0 if there was an error with the
 *         device.
 */
double receiver::set_input_rate(double rate)
{
    double  current_rate;
    bool    rate_has_changed;

    current_rate = src->get_sample_rate();
    rate_has_changed = !(rate == current_rate ||
            std::abs(rate - current_rate) < std::abs(std::min(rate, current_rate))
            * std::numeric_limits<double>::epsilon());

    tb->lock();
    try
    {
        d_input_rate = src->set_sample_rate(rate);
    }
    catch (std::runtime_error &e)
    {
        d_input_rate = 0;
    }

    if (d_input_rate == 0)
    {
        // This can be the case when no device is attached and gr-osmosdr
        // puts in a null_source with rate 100 ksps or if the rate has not
        // changed
        if (rate_has_changed)
        {
            std::cerr << std::endl;
            std::cerr << "Failed to set RX input rate to " << rate << std::endl;
            std::cerr << "Your device may not be working properly." << std::endl;
            std::cerr << std::endl;
        }
        d_input_rate = rate;
    }

    d_decim_rate = d_input_rate / (double)d_decim;
    dc_corr->set_sample_rate(d_decim_rate);
    for (auto& rxc : rx)
        rxc->set_quad_rate(d_decim_rate);
    iq_fft->set_quad_rate(d_decim_rate);
    tb->unlock();

    return d_input_rate;
}

/** Set input decimation */
unsigned int receiver::set_input_decim(unsigned int decim)
{
    if (decim == d_decim)
        return d_decim;

    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    if (d_decim >= 2)
    {
        tb->disconnect(src, 0, input_decim, 0);
        tb->disconnect(input_decim, 0, iq_swap, 0);
    }
    else
    {
        tb->disconnect(src, 0, iq_swap, 0);
    }

    input_decim.reset();
    d_decim = decim;
    if (d_decim >= 2)
    {
        try
        {
            input_decim = make_fir_decim_cc(d_decim);
        }
        catch (std::range_error &e)
        {
            std::cout << "Error opening creating input decimator " << d_decim
                      << ": " << e.what() << std::endl
                      << "Using decimation 1." << std::endl;
            d_decim = 1;
        }

        d_decim_rate = d_input_rate / (double)d_decim;
    }
    else
    {
        d_decim_rate = d_input_rate;
    }

    // update quadrature rate
    dc_corr->set_sample_rate(d_decim_rate);
    for (auto& rxc : rx)
        rxc->set_quad_rate(d_decim_rate);
    iq_fft->set_quad_rate(d_decim_rate);

    if (d_decim >= 2)
    {
        tb->connect(src, 0, input_decim, 0);
        tb->connect(input_decim, 0, iq_swap, 0);
    }
    else
    {
        tb->connect(src, 0, iq_swap, 0);
    }

#ifdef CUSTOM_AIRSPY_KERNELS
    if (input_devstr.find("airspy") != std::string::npos)
        src->set_bandwidth(d_decim_rate);
#endif

    if (d_running)
        tb->start();

    return d_decim;
}

/**
 * @brief Set new analog bandwidth.
 * @param bw The new bandwidth.
 * @return The actual bandwidth.
 */
double receiver::set_analog_bandwidth(double bw)
{
    return src->set_bandwidth(bw);
}

/** Get current analog bandwidth. */
double receiver::get_analog_bandwidth(void) const
{
    return src->get_bandwidth();
}

/** Set I/Q reversed. */
void receiver::set_iq_swap(bool reversed)
{
    if (reversed == d_iq_rev)
        return;

    d_iq_rev = reversed;
    iq_swap->set_enabled(d_iq_rev);
}

/**
 * @brief Get current I/Q reversed setting.
 * @retval true I/Q swappign is enabled.
 * @retval false I/Q swapping is disabled.
 */
bool receiver::get_iq_swap(void) const
{
    return d_iq_rev;
}

/**
 * @brief Enable/disable automatic DC removal in the I/Q stream.
 * @param enable Whether DC removal should enabled or not.
 */
void receiver::set_dc_cancel(bool enable)
{
    if (enable == d_dc_cancel)
        return;

    d_dc_cancel = enable;

    // until we have a way to switch on/off
    // inside the dc_corr_cc we do a reconf
    reconnect_all(true);
}

/**
 * @brief Get auto DC cancel status.
 * @retval true  Automatic DC removal is enabled.
 * @retval false Automatic DC removal is disabled.
 */
bool receiver::get_dc_cancel(void) const
{
    return d_dc_cancel;
}

/**
 * @brief Enable/disable automatic I/Q balance.
 * @param enable Whether automatic I/Q balance should be enabled.
 */
void receiver::set_iq_balance(bool enable)
{
    if (enable == d_iq_balance)
        return;

    d_iq_balance = enable;

    src->set_iq_balance_mode(enable ? 2 : 0);
}

/**
 * @brief Get auto I/Q balance status.
 * @retval true  Automatic I/Q balance is enabled.
 * @retval false Automatic I/Q balance is disabled.
 */
bool receiver::get_iq_balance(void) const
{
    return d_iq_balance;
}

/**
 * @brief Set RF frequency.
 * @param freq_hz The desired frequency in Hz.
 * @return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
 * @sa get_rf_freq()
 */
receiver::status receiver::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    src->set_center_freq(d_rf_freq);
    for (auto& rxc : rx)
        rxc->set_center_freq(d_rf_freq);//to generate audio filename
    // FIXME: read back frequency?

    return STATUS_OK;
}

/**
 * @brief Get RF frequency.
 * @return The current RF frequency.
 * @sa set_rf_freq()
 */
double receiver::get_rf_freq(void)
{
    d_rf_freq = src->get_center_freq();

    return d_rf_freq;
}

/**
 * @brief Get the RF frequency range of the current input device.
 * @param start The lower limit of the range in Hz.
 * @param stop  The upper limit of the range in Hz.
 * @param step  The frequency step in Hz.
 * @returns STATUS_OK if the range could be retrieved, STATUS_ERROR if an error has occurred.
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

/** Get the names of available gain stages. */
std::vector<std::string> receiver::get_gain_names()
{
    return src->get_gain_names();
}

/**
 * @brief Get gain range for a specific stage.
 * @param[in]  name The name of the gain stage.
 * @param[out] start Lower limit for this gain setting.
 * @param[out] stop  Upper limit for this gain setting.
 * @param[out] step  The resolution for this gain setting.
 *
 * This function returns the range for the requested gain stage.
 */
receiver::status receiver::get_gain_range(std::string &name, double *start,
                                          double *stop, double *step) const
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

double receiver::get_gain(std::string name) const
{
    return src->get_gain(name);
}

/**
 * @brief Set RF gain.
 * @param gain_rel The desired relative gain between 0.0 and 1.0 (use -1 for
 *                 AGC where supported).
 * @return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
 */
receiver::status receiver::set_auto_gain(bool automatic)
{
    src->set_gain_mode(automatic);

    return STATUS_OK;
}

/**
 * @brief Add new demodulator and select it
 * @return current rx index or -1 if an error occurs.
 */
int receiver::add_rx()
{
    if (rx.size() == RX_MAX)
        return -1;
    //tb->lock(); does not allow block input count changing
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    if (d_current >= 0)
        background_rx();
    rx.push_back(make_nbrx(d_decim_rate, d_audio_rate));
    int old = d_current;
    d_current = rx.size() - 1;
    rx[d_current]->set_index(d_current);
    rx[d_current]->set_rec_event_handler(std::bind(audio_rec_event, this,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3));
    set_demod_locked(rx[old]->get_demod(), old);
    if (d_running)
        tb->start();
    return d_current;
}

/**
 * @brief Get demodulator count
 * @return demodulator count.
 */
int receiver::get_rx_count()
{
    return rx.size();
}

/**
 * @brief Delete selected demodulator, select nearest remaining demodulator.
 * @return selected rx index or -1 if there are 0 demodulators remaining.
 */
int receiver::delete_rx()
{
    if (d_current == -1)
        return -1;
    if (rx.size() <= 1)
        return 0;
    //tb->lock(); does not allow block input count changing
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    background_rx();
    disconnect_rx();
    rx[d_current].reset();
    if (rx.size() > 1)
    {
        if (d_current != int(rx.size()) - 1)
        {
            rx[d_current] = rx.back();
            rx[d_current]->set_index(d_current);
            rx.back().reset();
        }
        else
            d_current--;
    }
    else
    {
        d_current = -1;
    }
    rx.pop_back();
    foreground_rx();
    if (d_running)
        tb->start();
    return d_current;
}

/**
 * @brief Selects a demodulator.
 * @return STATUS_OK or STATUS_ERROR (if requested demodulator does not exist).
 */
receiver::status receiver::select_rx(int no)
{
    if (no == d_current)
        return STATUS_OK;
    if (no < int(rx.size()))
    {
        tb->lock();
        if (d_current >= 0)
            background_rx();
        d_current = no;
        foreground_rx();
        tb->unlock();
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

receiver::status receiver::fake_select_rx(int no)
{
    if (no == d_current)
        return STATUS_OK;
    if (no < int(rx.size()))
    {
        d_current = no;
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

int receiver::get_current()
{
    return d_current;
}

vfo::sptr receiver::get_current_vfo()
{
    return get_vfo(d_current);
}

vfo::sptr receiver::find_vfo(int64_t freq)
{
    vfo::sptr notfound;
    int64_t offset = freq - d_rf_freq;
    //FIXME: speedup with index???
    for (auto& rxc : rx)
        if (rxc->get_offset() == offset)
            return rxc;
    return notfound;
}

vfo::sptr receiver::get_vfo(int n)
{
    return rx[n];
}

std::vector<vfo::sptr> receiver::get_vfos()
{
    std::vector<vfo::sptr> vfos;
    vfos.reserve(rx.size());
    for (auto& rxc : rx)
        vfos.push_back(rxc);
    return vfos;
}

/**
 * @brief Set filter offset.
 * @param offset_hz The desired filter offset in Hz.
 * @return RX_STATUS_ERROR if the tuning offset is out of range.
 *
 * This method sets a new tuning offset for the receiver. The tuning offset is used
 * to tune within the passband, i.e. select a specific channel within the received
 * spectrum.
 *
 * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
 * logical limit.
 *
 * @sa get_filter_offset()
 */
receiver::status receiver::set_filter_offset(double offset_hz)
{
    set_filter_offset(d_current, offset_hz);
    return STATUS_OK;
}

receiver::status receiver::set_filter_offset(int rx_index, double offset_hz)
{
    rx[rx_index]->set_offset(offset_hz);//to generate audio filename from
    if (rx[rx_index]->get_agc_panning_auto())
        rx[rx_index]->set_agc_panning(offset_hz * 200.0 / d_decim_rate);

    return STATUS_OK;
}

/**
 * @brief Get filter offset.
 * @return The current filter offset.
 * @sa set_filter_offset()
 */
double receiver::get_filter_offset(void) const
{
    return rx[d_current]->get_offset();
}

void receiver::set_freq_lock(bool on, bool all)
{
    if (all)
        for (auto& rxc : rx)
            rxc->set_freq_lock(on);
    else
        rx[d_current]->set_freq_lock(on);
}

bool receiver::get_freq_lock()
{
    return rx[d_current]->get_freq_lock();
}


/* CW offset can serve as a "BFO" if the GUI needs it */
receiver::status receiver::set_cw_offset(double offset_hz)
{
    rx[d_current]->set_cw_offset(offset_hz);

    return STATUS_OK;
}

double receiver::get_cw_offset(void) const
{
    return rx[d_current]->get_cw_offset();
}

receiver::status receiver::set_filter(int low, int high, filter_shape shape)
{
    if ((low >= high) || (std::abs(high-low) < RX_FILTER_MIN_WIDTH))
        return STATUS_ERROR;

    rx[d_current]->set_filter(low, high, Modulations::TwFromFilterShape(low, high, shape));
    return STATUS_OK;
}

receiver::status receiver::get_filter(int &low, int &high, filter_shape &shape)
{
    int tw;
    rx[d_current]->get_filter(low, high, tw);
    shape = Modulations::FilterShapeFromTw(low, high, tw);
    return STATUS_OK;
}

receiver::status receiver::set_freq_corr(double ppm)
{
    src->set_freq_corr(ppm);

    return STATUS_OK;
}

/**
 * @brief Get current signal power.
 * @param dbfs Whether to use dbfs or absolute power.
 * @return The current signal power.
 *
 * This method returns the current signal power detected by the receiver. The detector
 * is located after the band pass filter. The full scale is 1.0
 */
float receiver::get_signal_pwr() const
{
    return rx[d_current]->get_signal_level();
}

/** Set new FFT size. */
void receiver::set_iq_fft_size(int newsize)
{
    iq_fft->set_fft_size(newsize);
}

unsigned int receiver::iq_fft_size() const
{
    return iq_fft->fft_size();
}

void receiver::set_iq_fft_window(int window_type, bool normalize_energy)
{
    iq_fft->set_window_type(window_type, normalize_energy);
}

/** Get latest baseband FFT data. */
int receiver::get_iq_fft_data(float* fftPoints)
{
    return iq_fft->get_fft_data(fftPoints);
}

unsigned int receiver::audio_fft_size() const
{
    return audio_fft->fft_size();
}

/** Get latest audio FFT data. */
int receiver::get_audio_fft_data(float* fftPoints)
{
    return audio_fft->get_fft_data(fftPoints);
}

receiver::status receiver::set_nb_on(int nbid, bool on)
{
    rx[d_current]->set_nb_on(nbid, on);

    return STATUS_OK; // FIXME
}

bool receiver::get_nb_on(int nbid)
{
    return rx[d_current]->get_nb_on(nbid);
}

receiver::status receiver::set_nb_threshold(int nbid, float threshold)
{
    rx[d_current]->set_nb_threshold(nbid, threshold);

    return STATUS_OK; // FIXME
}

float receiver::get_nb_threshold(int nbid)
{
    return rx[d_current]->get_nb_threshold(nbid);
}

/**
 * @brief Set squelch level.
 * @param level_db The new level in dBFS.
 */
receiver::status receiver::set_sql_level(float level_db)
{
    rx[d_current]->set_sql_level(level_db);

    return STATUS_OK; // FIXME
}

receiver::status receiver::set_sql_level(float level_offset, bool global, bool relative)
{
    if (global)
        for (auto& rxc: rx)
            rxc->set_sql_level((relative ? rxc->get_signal_level() : 0.f) + level_offset);
    else
        rx[d_current]->set_sql_level((relative ? rx[d_current]->get_signal_level() : 0.f) + level_offset);

    return STATUS_OK; // FIXME
}

double receiver::get_sql_level()
{
    return rx[d_current]->get_sql_level();
}

/** Set squelch alpha */
receiver::status receiver::set_sql_alpha(double alpha)
{
    rx[d_current]->set_sql_alpha(alpha);

    return STATUS_OK; // FIXME
}

double receiver::get_sql_alpha()
{
    return rx[d_current]->get_sql_alpha();
}

/**
 * @brief Enable/disable receiver AGC.
 *
 * When AGC is disabled a fixed manual gain is used, see set_agc_manual_gain().
 */
receiver::status receiver::set_agc_on(bool agc_on)
{
    rx[d_current]->set_agc_on(agc_on);

    return STATUS_OK; // FIXME
}

bool receiver::get_agc_on()
{
    return rx[d_current]->get_agc_on();
}

/** Set AGC hang. */
receiver::status receiver::set_agc_hang(int hang_ms)
{
    rx[d_current]->set_agc_hang(hang_ms);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_hang()
{
    return rx[d_current]->get_agc_hang();
}

/** Set AGC target level. */
receiver::status receiver::set_agc_target_level(int target_level)
{
    rx[d_current]->set_agc_target_level(target_level);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_target_level()
{
    return rx[d_current]->get_agc_target_level();
}

/** Set fixed gain used when AGC is OFF. */
receiver::status receiver::set_agc_manual_gain(float gain)
{
    rx[d_current]->set_agc_manual_gain(gain);

    return STATUS_OK; // FIXME
}

float receiver::get_agc_manual_gain()
{
    return rx[d_current]->get_agc_manual_gain();
}

/** Set maximum gain used when AGC is ON. */
receiver::status receiver::set_agc_max_gain(int gain)
{
    rx[d_current]->set_agc_max_gain(gain);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_max_gain()
{
    return rx[d_current]->get_agc_max_gain();
}

/** Set AGC attack. */
receiver::status receiver::set_agc_attack(int attack_ms)
{
    rx[d_current]->set_agc_attack(attack_ms);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_attack()
{
    return rx[d_current]->get_agc_attack();
}

/** Set AGC decay time. */
receiver::status receiver::set_agc_decay(int decay_ms)
{
    rx[d_current]->set_agc_decay(decay_ms);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_decay()
{
    return rx[d_current]->get_agc_decay();
}

/** Set AGC panning. */
receiver::status receiver::set_agc_panning(int panning)
{
    rx[d_current]->set_agc_panning(panning);

    return STATUS_OK; // FIXME
}

int receiver::get_agc_panning()
{
    return rx[d_current]->get_agc_panning();
}

/** Set AGC panning auto mode. */
receiver::status receiver::set_agc_panning_auto(bool mode)
{
    rx[d_current]->set_agc_panning_auto(mode);
    if (mode)
        rx[d_current]->set_agc_panning(rx[d_current]->get_offset() * 200.0 / d_decim_rate);

    return STATUS_OK; // FIXME
}

bool receiver::get_agc_panning_auto()
{
    return rx[d_current]->get_agc_panning_auto();
}

/** Set AGC mute */
receiver::status receiver::set_agc_mute(bool agc_mute)
{
    rx[d_current]->set_agc_mute(agc_mute);

    return STATUS_OK; // FIXME
}

bool receiver::get_agc_mute()
{
    return rx[d_current]->get_agc_mute();
}

/** Get AGC current gain. */
float receiver::get_agc_gain()
{
    return rx[d_current]->get_agc_gain();
}

/** Set audio mute. */
receiver::status receiver::set_mute(bool mute)
{
    if (d_mute == mute)
        return STATUS_OK;
    d_mute = mute;
    if (d_mute)
    {
        mc0->set_k(0);
        mc1->set_k(0);
    }
    else
    {
        float mul_k = get_rx_count() ? 1.f / float(get_rx_count()) : 1.f;
        mc0->set_k(mul_k);
        mc1->set_k(mul_k);
    }
    return STATUS_OK;
}

/** Get audio mute. */
bool receiver::get_mute()
{
    return d_mute;
}

receiver::status receiver::set_demod_locked(Modulations::idx demod, int old_idx)
{
    status ret = STATUS_OK;
    rx_chain rxc = RX_CHAIN_NONE;
    if (old_idx == -1)
    {
        background_rx();
        disconnect_rx();
    }

    switch (demod)
    {
    case Modulations::MODE_OFF:
        rxc = RX_CHAIN_NONE;
        break;

    case Modulations::MODE_RAW:
    case Modulations::MODE_AM:
    case Modulations::MODE_AM_SYNC:
    case Modulations::MODE_NFM:
    case Modulations::MODE_LSB:
    case Modulations::MODE_USB:
    case Modulations::MODE_CWL:
    case Modulations::MODE_CWU:
        rxc = RX_CHAIN_NBRX;
        break;

    case Modulations::MODE_WFM_MONO:
    case Modulations::MODE_WFM_STEREO:
    case Modulations::MODE_WFM_STEREO_OIRT:
        rxc = RX_CHAIN_WFMRX;
        break;

    default:
        ret = STATUS_ERROR;
        break;
    }
    if (ret != STATUS_ERROR)
    {
        receiver_base_cf_sptr old_rx = rx[(old_idx == -1) ? d_current : old_idx];
        // RX demod chain
        switch (rxc)
        {
        case RX_CHAIN_NBRX:
        case RX_CHAIN_NONE:
            if (rx[d_current]->name() != "NBRX")
            {
                rx[d_current].reset();
                rx[d_current] = make_nbrx(d_decim_rate, d_audio_rate);
                rx[d_current]->set_index(d_current);
                rx[d_current]->set_rec_event_handler(std::bind(audio_rec_event, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
            }
            break;

        case RX_CHAIN_WFMRX:
            if (rx[d_current]->name() != "WFMRX")
            {
                rx[d_current].reset();
                rx[d_current] = make_wfmrx(d_decim_rate, d_audio_rate);
                rx[d_current]->set_index(d_current);
                rx[d_current]->set_rec_event_handler(std::bind(audio_rec_event, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
            }
            break;

        default:
            break;
        }
        //Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
        tb->connect(iq_src, 0, rx[d_current], 0);
        // End temporary workaronud
        if (old_rx.get() != rx[d_current].get())
        {
            rx[d_current]->restore_settings(*old_rx.get());
            // Recorders
            if (old_rx.get() != rx[d_current].get())
            {
                if (old_idx == -1)
                    if (old_rx->get_audio_recording())
                        rx[d_current]->continue_audio_recording(old_rx);
            }
            if (demod == Modulations::MODE_OFF)
            {
                if (rx[d_current]->get_audio_recording())
                {
                    rx[d_current]->stop_audio_recording();
                }
            }
        }
        rx[d_current]->set_demod(demod);
        //Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
        tb->disconnect(iq_src, 0, rx[d_current], 0);
        // End temporary workaronud
    }
    connect_rx();
    foreground_rx();
    return ret;
}

receiver::status receiver::set_demod(Modulations::idx demod, int old_idx)
{
    status ret = STATUS_OK;
    if (rx[d_current]->get_demod() == demod)
        return ret;
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    ret = set_demod_locked(demod, old_idx);
    if (d_running)
        tb->start();

    return ret;
}

receiver::status receiver::reconnect_all(bool force)
{
    status ret = STATUS_OK;
    // tb->lock() seems to hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }
    if (force)
    {
        tb->disconnect_all();
        for (auto& rxc : rx)
            rxc->connected(false);
    }
    connect_all();

    if (d_running)
        tb->start();

    return ret;
}


/**
 * @brief Set maximum deviation of the FM demodulator.
 * @param maxdev_hz The new maximum deviation in Hz.
 */
receiver::status receiver::set_fm_maxdev(float maxdev_hz)
{
    rx[d_current]->set_fm_maxdev(maxdev_hz);

    return STATUS_OK;
}

float receiver::get_fm_maxdev()
{
    return rx[d_current]->get_fm_maxdev();
}

receiver::status receiver::set_fm_deemph(double tau)
{
    rx[d_current]->set_fm_deemph(tau);

    return STATUS_OK;
}

double receiver::get_fm_deemph()
{
    return rx[d_current]->get_fm_deemph();
}

receiver::status receiver::set_am_dcr(bool enabled)
{
    rx[d_current]->set_am_dcr(enabled);

    return STATUS_OK;
}

bool receiver::get_am_dcr()
{
    return rx[d_current]->get_am_dcr();
}

receiver::status receiver::set_amsync_dcr(bool enabled)
{
    rx[d_current]->set_amsync_dcr(enabled);

    return STATUS_OK;
}

bool receiver::get_amsync_dcr()
{
    return rx[d_current]->get_amsync_dcr();
}

receiver::status receiver::set_amsync_pll_bw(float pll_bw)
{
    rx[d_current]->set_amsync_pll_bw(pll_bw);

    return STATUS_OK;
}

float receiver::get_amsync_pll_bw()
{
    return rx[d_current]->get_amsync_pll_bw();
}

receiver::status receiver::set_audio_rec_dir(const std::string dir)
{
    //FIXME is it a global option, that should be set with for-loop?
    rx[d_current]->set_audio_rec_dir(dir);
    return STATUS_OK;
}

std::string receiver::get_audio_rec_dir()
{
    //FIXME is it a global option, that should be set with for-loop?
    return rx[d_current]->get_audio_rec_dir();
}

receiver::status receiver::set_audio_rec_sql_triggered(const bool enabled)
{
    rx[d_current]->set_audio_rec_sql_triggered(enabled);
    return STATUS_OK;
}

bool receiver::get_audio_rec_sql_triggered()
{
    return rx[d_current]->get_audio_rec_sql_triggered();
}

receiver::status receiver::set_audio_rec_min_time(const int time_ms)
{
    rx[d_current]->set_audio_rec_min_time(time_ms);
    return STATUS_OK;
}

int receiver::get_audio_rec_min_time()
{
    return rx[d_current]->get_audio_rec_min_time();
}

receiver::status receiver::set_audio_rec_max_gap(const int time_ms)
{
    rx[d_current]->set_audio_rec_max_gap(time_ms);
    return STATUS_OK;
}

int receiver::get_audio_rec_max_gap()
{
    return rx[d_current]->get_audio_rec_max_gap();
}

/**
 * @brief Start WAV file recorder.
 * @param filename The filename where to record.
 *
 * A new recorder object is created every time we start recording and deleted every time
 * we stop recording. The idea of creating one object and starting/stopping using different
 * file names does not work with WAV files (the initial /tmp/gqrx.wav will not be stopped
 * because the wav file can not be empty). See https://github.com/gqrx-sdr/gqrx/issues/36
 */
receiver::status receiver::start_audio_recording()
{
    if (is_recording_audio())
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

    if (rx[d_current]->start_audio_recording() == 0)
        return STATUS_OK;
    else
        return STATUS_ERROR;
}

/** Stop WAV file recorder. */
receiver::status receiver::stop_audio_recording()
{
    if (!is_recording_audio()){
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
    rx[d_current]->stop_audio_recording();

    return STATUS_OK;
}

/** get last recorded audio file name. */
std::string receiver::get_last_audio_filename()
{
    return rx[d_current]->get_last_audio_filename();
}

/** Start audio playback. */
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
    if (d_active > 0)
    {
        tb->disconnect(mc0, 0, audio_snk, 0);
        tb->disconnect(mc1, 0, audio_snk, 1);
    }
    tb->disconnect(rx[d_current], 0, audio_fft, 0);
    tb->connect(rx[d_current], 0, audio_null_sink0, 0); /** FIXME: other channel? */
    tb->connect(rx[d_current], 1, audio_null_sink1, 0); /** FIXME: other channel? */
    tb->connect(wav_src, 0, audio_snk, 0);
    tb->connect(wav_src, 1, audio_snk, 1);
    tb->connect(wav_src, 0, audio_fft, 0);
    start();

    std::cout << "Playing audio from " << filename << std::endl;

    return STATUS_OK;
}

/** Stop audio playback. */
receiver::status receiver::stop_audio_playback()
{
    /* disconnect wav source and reconnect receiver */
    stop();
    tb->disconnect(wav_src, 0, audio_snk, 0);
    tb->disconnect(wav_src, 1, audio_snk, 1);
    tb->disconnect(wav_src, 0, audio_fft, 0);
    tb->disconnect(rx[d_current], 0, audio_null_sink0, 0);
    tb->disconnect(rx[d_current], 1, audio_null_sink1, 0);
    if (d_active > 0)
    {
        tb->connect(mc0, 0, audio_snk, 0);
        tb->connect(mc1, 0, audio_snk, 1);
    }
    tb->connect(rx[d_current], 0, audio_fft, 0);  /** FIXME: other channel? */
    start();

    /* delete wav_src since we can not change file name */
    wav_src.reset();

    return STATUS_OK;
}

/** UDP streaming of audio. */
receiver::status receiver::set_udp_host(std::string host)
{
    return rx[d_current]->set_udp_host(host) ? STATUS_OK : STATUS_ERROR;
}

std::string receiver::get_udp_host()
{
    return rx[d_current]->get_udp_host();
}

receiver::status receiver::set_udp_port(int port)
{
    return rx[d_current]->set_udp_port(port) ? STATUS_OK : STATUS_ERROR;
}

int receiver::get_udp_port()
{
    return rx[d_current]->get_udp_port();
}

receiver::status receiver::set_udp_stereo(bool stereo)
{
    return rx[d_current]->set_udp_stereo(stereo) ? STATUS_OK : STATUS_ERROR;
}

bool receiver::get_udp_stereo()
{
    return rx[d_current]->get_udp_stereo();
}

receiver::status receiver::set_udp_streaming(bool streaming)
{
    return rx[d_current]->set_udp_streaming(streaming) ? STATUS_OK : STATUS_ERROR;
}

bool receiver::get_udp_streaming()
{
    return rx[d_current]->get_udp_streaming();
}

/**
 * @brief Start I/Q data recorder.
 * @param filename The filename where to record.
 */
receiver::status receiver::start_iq_recording(const std::string filename)
{
    receiver::status status = STATUS_OK;

    if (d_recording_iq) {
        std::cout << __func__ << ": already recording" << std::endl;
        return STATUS_ERROR;
    }

    try
    {
        iq_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename.c_str(), true);
    }
    catch (std::runtime_error &e)
    {
        std::cout << __func__ << ": couldn't open I/Q file" << std::endl;
        return STATUS_ERROR;
    }

    tb->lock();
    if (d_decim >= 2)
        tb->connect(input_decim, 0, iq_sink, 0);
    else
        tb->connect(src, 0, iq_sink, 0);
    d_recording_iq = true;
    tb->unlock();

    return status;
}

/** Stop I/Q data recorder. */
receiver::status receiver::stop_iq_recording()
{
    if (!d_recording_iq) {
        /* error: we are not recording */
        return STATUS_ERROR;
    }

    tb->lock();
    iq_sink->close();

    if (d_decim >= 2)
        tb->disconnect(input_decim, 0, iq_sink, 0);
    else
        tb->disconnect(src, 0, iq_sink, 0);

    tb->unlock();
    iq_sink.reset();
    d_recording_iq = false;

    return STATUS_OK;
}

/**
 * @brief Seek to position in IQ file source.
 * @param pos Byte offset from the beginning of the file.
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

/**
 * @brief Start data sniffer.
 * @param buffsize The buffer that should be used in the sniffer.
 * @return STATUS_OK if the sniffer was started, STATUS_ERROR if the sniffer is already in use.
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
    tb->connect(rx[d_current], 0, sniffer_rr, 0);
    tb->connect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = true;

    return STATUS_OK;
}

/**
 * @brief Stop data sniffer.
 * @return STATUS_ERROR i the sniffer is not currently active.
 */
receiver::status receiver::stop_sniffer()
{
    if (!d_sniffer_active) {
        return STATUS_ERROR;
    }

    tb->lock();
    tb->disconnect(rx[d_current], 0, sniffer_rr, 0);

    // Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
    tb->disconnect(rx[d_current], 0, audio_fft, 0);
    tb->connect(rx[d_current], 0, audio_fft, 0);
    // End temporary workaronud

    tb->disconnect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = false;

    /* delete resampler */
    sniffer_rr.reset();

    return STATUS_OK;
}

/** Get sniffer data. */
void receiver::get_sniffer_data(float * outbuff, unsigned int &num)
{
    sniffer->get_samples(outbuff, num);
}

/** Convenience function to connect all blocks. */
void receiver::connect_all()
{
    gr::basic_block_sptr b;

    // Setup source
    b = src;

    // Pre-processing
    if (d_decim >= 2)
    {
        tb->connect(b, 0, input_decim, 0);
        b = input_decim;
    }

    if (d_recording_iq)
    {
        // We record IQ with minimal pre-processing
        tb->connect(b, 0, iq_sink, 0);
    }

    tb->connect(b, 0, iq_swap, 0);
    b = iq_swap;

    if (d_dc_cancel)
    {
        tb->connect(b, 0, dc_corr, 0);
        b = dc_corr;
    }

    // Visualization
    tb->connect(b, 0, iq_fft, 0);
    iq_src = b;

    // Audio path (if there is a receiver)
    d_active = 0;
    std::cerr<<"connect_all "<<get_rx_count()<<std::endl;
    for (auto& rxc : rx)
        connect_rx(rxc->get_index());
    foreground_rx();
}

void receiver::connect_rx()
{
    connect_rx(d_current);
}

void receiver::connect_rx(int n)
{
    if (!rx[n])
        return;
    if (rx[n]->connected())
        return;
    std::cerr<<"connect_rx "<<n<<" active "<<d_active<<" demod "<<rx[n]->get_demod()<<std::endl;
    if (rx[n]->get_demod() != Modulations::MODE_OFF)
    {
        if (d_active == 0)
        {
           std::cerr<<"connect_rx d_active == 0"<<std::endl;
            for (auto& rxc : rx)
            {
                if (!rxc)
                    continue;
                std::cerr<<"connect_rx loop "<<rxc->get_index()<<std::endl;
                if (rxc->get_demod() != Modulations::MODE_OFF)
                {
                        std::cerr<<"connect_rx loop !MODE_OFF"<<std::endl;
                    tb->connect(iq_src, 0, rxc, 0);
                    tb->connect(rxc, 0, add0, rxc->get_index());
                    tb->connect(rxc, 1, add1, rxc->get_index());
                    d_active++;
                }
                else
                {
                        std::cerr<<"connect_rx loop MODE_OFF"<<std::endl;
                    tb->connect(null_src, 0, add0, rxc->get_index());
                    tb->connect(null_src, 0, add1, rxc->get_index());
                }
                rxc->connected(true);
            }
            std::cerr<<"connect_rx connect add"<<std::endl;
            tb->connect(add0, 0, mc0, 0);
            tb->connect(add1, 0, mc1, 0);
            std::cerr<<"connect audio_snk "<<d_active<<std::endl;
            tb->connect(mc0, 0, audio_snk, 0);
            tb->connect(mc1, 0, audio_snk, 1);
        }
        else
        {
            std::cerr<<"connect_rx d_active > 0"<<std::endl;
            tb->connect(iq_src, 0, rx[n], 0);
            tb->connect(rx[n], 0, add0, n);
            tb->connect(rx[n], 1, add1, n);
            rx[n]->connected(true);
            d_active++;
        }
    }
    else
    {
        if (d_active > 0)
        {
            std::cerr<<"connect_rx MODE_OFF d_active > 0"<<std::endl;
            tb->connect(null_src, 0, add0, n);
            tb->connect(null_src, 0, add1, n);
            rx[n]->connected(true);
        }
    }
}

void receiver::disconnect_rx()
{
    disconnect_rx(d_current);
}

void receiver::disconnect_rx(int n)
{
    std::cerr<<"disconnect_rx "<<n<<" active "<<d_active<<" demod "<<rx[n]->get_demod()<<std::endl;
    if (rx[n]->get_demod() != Modulations::MODE_OFF)
    {
        d_active--;
        if (d_active == 0)
        {
           std::cerr<<"disconnect_rx d_active == 0"<<std::endl;
            for (auto& rxc : rx)
            {
                if (!rxc)
                    continue;
                std::cerr<<"disconnect_rx loop "<<rxc->get_index()<<std::endl;
                if (rxc->connected())
                {
                    if (rxc->get_demod() != Modulations::MODE_OFF)
                    {
                        std::cerr<<"disconnect_rx loop !MODE_OFF"<<std::endl;
                        tb->disconnect(iq_src, 0, rxc, 0);
                        tb->disconnect(rxc, 0, add0, rxc->get_index());
                        tb->disconnect(rxc, 1, add1, rxc->get_index());
                    }
                    else
                    {
                        std::cerr<<"disconnect_rx loop MODE_OFF"<<std::endl;
                        tb->disconnect(null_src, 0, add0, rxc->get_index());
                        tb->disconnect(null_src, 0, add1, rxc->get_index());
                    }
                    rxc->connected(false);
                }
            }
            std::cerr<<"disconnect_rx disconnect add"<<std::endl;
            tb->disconnect(add0, 0, mc0, 0);
            tb->disconnect(add1, 0, mc1, 0);
            std::cerr<<"disconnect audio_snk "<<d_active<<std::endl;
            tb->disconnect(mc0, 0, audio_snk, 0);
            tb->disconnect(mc1, 0, audio_snk, 1);
        }
        else
        {
            std::cerr<<"disconnect_rx d_active > 0"<<std::endl;
            tb->disconnect(iq_src, 0, rx[n], 0);
            tb->disconnect(rx[n], 0, add0, n);
            tb->disconnect(rx[n], 1, add1, n);
        }
    }
    else
    {
        if (d_active > 0)
        {
            std::cerr<<"disconnect_rx MODE_OFF d_active > 0"<<std::endl;
            tb->disconnect(null_src, 0, add0, n);
            tb->disconnect(null_src, 0, add1, n);
        }
    }
    rx[n]->connected(false);
}

void receiver::background_rx()
{
    std::cerr<<"background_rx "<<d_current<<" "<<rx[d_current]->get_demod()<<std::endl;
    if (rx[d_current]->get_demod() != Modulations::MODE_OFF)
    {
        tb->disconnect(rx[d_current], 0, audio_fft, 0);
        if (d_sniffer_active)
        {
            tb->disconnect(rx[d_current], 0, sniffer_rr, 0);
            tb->disconnect(sniffer_rr, 0, sniffer, 0);
        }
    }
}

void receiver::foreground_rx()
{
    std::cerr<<"foreground_rx "<<d_current<<" "<<rx[d_current]->get_demod()<<std::endl;
    if (rx[d_current]->get_demod() != Modulations::MODE_OFF)
    {
        tb->connect(rx[d_current], 0, audio_fft, 0);
        if (d_sniffer_active)
        {
            tb->connect(rx[d_current], 0, sniffer_rr, 0);
            tb->connect(sniffer_rr, 0, sniffer, 0);
        }
    }
    else
    {

        if (d_sniffer_active)
        {
            d_sniffer_active = false;

            /* delete resampler */
            sniffer_rr.reset();
        }
    }
    d_mute = !d_mute;
    set_mute(!d_mute);
}

void receiver::get_rds_data(std::string &outbuff, int &num)
{
    rx[d_current]->get_rds_data(outbuff, num);
}

void receiver::start_rds_decoder(void)
{
    if (is_rds_decoder_active())
        return;
    if (d_running)
    {
        stop();
        rx[d_current]->start_rds_decoder();
        start();
    }
    else
    {
        rx[d_current]->start_rds_decoder();
    }
}

void receiver::stop_rds_decoder(void)
{
    if (!is_rds_decoder_active())
        return;
    if (d_running)
    {
        stop();
        rx[d_current]->stop_rds_decoder();
        start();
    }
    else
    {
        rx[d_current]->stop_rds_decoder();
    }
}

bool receiver::is_rds_decoder_active(void) const
{
    return rx[d_current]->is_rds_decoder_active();
}

void receiver::reset_rds_parser(void)
{
    rx[d_current]->reset_rds_parser();
}

std::string receiver::escape_filename(std::string filename)
{
    std::stringstream ss1;
    std::stringstream ss2;

    ss1 << std::quoted(filename, '\'', '\\');
    ss2 << std::quoted(ss1.str(), '\'', '\\');
    return ss2.str();
}

void receiver::audio_rec_event(receiver * self, int idx, std::string filename, bool is_running)
{
    if (is_running)
        std::cout << "Recording audio to " << filename << std::endl;
    else
        std::cout << "Audio recorder stopped" << std::endl;

    if (self->d_audio_rec_event_handler)
        if (idx == self->d_current)
            self->d_audio_rec_event_handler(filename, is_running);
}
