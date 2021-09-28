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
#include <iostream>

#include <gnuradio/prefs.h>
#include <QDebug>

#include "applications/gqrx/receiver.h"

#define TARGET_QUAD_RATE 1e6

/**
 * @brief Public constructor.
 * @param input_device Input device specifier.
 * @param audio_device Audio output device specifier,
 *                     e.g. hw:0 when using ALSA or Portaudio.
 */
receiver::receiver(const std::string input_device,
                   const std::string audio_device,
                   unsigned int decimation)
    : d_running(false),
      d_input_rate(96000.0),
      d_audio_rate(48000),
      d_decim(decimation),
      d_rf_freq(144800000.0),
      d_iq_rev(false),
      d_dc_cancel(false),
      d_iq_balance(false) /*,
      d_recording_iq(false) */
{

    tb = gr::make_top_block("gqrx");

    if (input_device.empty())
    {
        src = osmosdr::source::make("file="+get_random_file()+",freq=428e6,rate=96000,repeat=true,throttle=true");
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

    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;

    iq_swap = make_iq_swap_cc(false);
    dc_corr = make_dc_corr_cc(d_decim_rate, 1.0);
    iq_fft = make_rx_fft_c(8192u, d_decim_rate, gr::fft::window::WIN_HANN);

    output_devstr = audio_device;

    gr::prefs pref;
    qInfo() << "Using audio backend:"
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
    qInfo() << "Set input device:";
    qInfo() << "  old:" << input_devstr.c_str();
    qInfo() << "  new:" << device.c_str();

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

    // XXX somewhat duplicate operations in set_demod
    tb->disconnect_all();  // !!! surely not! - makes us use force below!
    qInfo() << "**** set_input_device disconnected all";

    src.reset();

    try
    {
        src = osmosdr::source::make(device);
    }
    catch (std::exception &x)
    {
        error = x.what();
        src = osmosdr::source::make("file="+get_random_file()+",freq=428e6,rate=96000,repeat=true,throttle=true");
    }

    if (src->get_sample_rate() != 0)
    {
        set_input_rate(src->get_sample_rate());
    }

    connect_all();
    qInfo() << "set_input_device connected all";

    // reset subreceivers
    for (size_t i = 0; i < demods.size(); ++i)
    {
        // TODO: do not force
        demods[i]->set_demod(demods[i]->get_demod(), true, subrxsrc, d_quad_rate, d_audio_rate);
        qInfo() << "set_input_device>set_demod for subrx" << i;
    }

    qInfo() << "\n\n" << gr::dot_graph(tb).c_str() << "\n\n";

    if (d_running)
    {
        tb->start();
    }

    qInfo() << "set_input_device done";

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
    qInfo() << "Set output device:";
    qInfo() << "   old:" << output_devstr.c_str();
    qInfo() << "   new:" << device.c_str();

    output_devstr = device;

    tb->lock();

    try {
        for (size_t i = 0; i < demods.size(); ++i)
        {
            demods[i]->set_output_device(device, d_audio_rate);
        }

        tb->unlock();

    } catch (std::exception &x) {
        tb->unlock();
        // handle problems on non-freeing devices
        throw x;
    }

    qInfo() << "receiver set_output_device done";
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
    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;
    dc_corr->set_sample_rate(d_decim_rate);

    for (size_t i = 0; i < demods.size(); ++i)
    {
        demods[i]->set_input_rate(d_ddc_decim, d_decim_rate, d_quad_rate);
    }

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
    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;
    dc_corr->set_sample_rate(d_decim_rate);

    for (size_t i = 0; i < demods.size(); ++i)
    {
        demods[i]->set_input_rate(d_ddc_decim, d_decim_rate, d_quad_rate);
    }

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
    qInfo() << "reciever set_dc_cancel starts";

    if (enable == d_dc_cancel)
        return;

    d_dc_cancel = enable;

    // until we have a way to switch on/off
    // inside the dc_corr_cc we do a reconfigure
    begin_reconfigure();
    for (size_t i = 0; i < demods.size(); ++i)
    {
        qInfo() << "reciever set_dc_cancel calls subrx" << i << "set_demod";
        demods[i]->set_demod(demods[i]->get_demod(), true, subrxsrc, d_quad_rate, d_audio_rate);
        qInfo() << "reciever set_dc_cancel calls subrx" << i << "set_demod done";
    }
    complete_reconfigure();

    qInfo() << "reciever set_dc_cancel done";
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
rx_status receiver::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    src->set_center_freq(d_rf_freq);
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
rx_status receiver::get_rf_range(double *start, double *stop, double *step)
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
rx_status receiver::get_gain_range(std::string &name, double *start,
                                          double *stop, double *step) const
{
    osmosdr::gain_range_t range;

    range = src->get_gain_range(name);
    *start = range.start();
    *stop  = range.stop();
    *step  = range.step();

    return STATUS_OK;
}

rx_status receiver::set_gain(std::string name, double value)
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
rx_status receiver::set_auto_gain(bool automatic)
{
    src->set_gain_mode(automatic);

    return STATUS_OK;
}

demodulator::sptr receiver::add_demodulator()
{
    qInfo() << "receiver add_rx begin";

    begin_reconfigure();

    auto nextIdx = demods.size();
    auto nextSub = std::make_shared<demodulator>(
        tb, subrxsrc,
        output_devstr, nextIdx,
        d_ddc_decim, d_decim_rate,
        d_quad_rate, d_audio_rate
    );
    qInfo() << "receiver add_rx created sub";
    demods.push_back(nextSub);

    for (size_t i = 0; i < demods.size(); ++i)
    {
        demods[i]->set_idx(i); // re-index
        qInfo() << "reciever remove_rx calls subrx" << i << "set_demod";
        demods[i]->set_demod(demods[i]->get_demod(), true, subrxsrc, d_quad_rate, d_audio_rate);
        qInfo() << "reciever remove_rx calls subrx" << i << "set_demod done";
    }

    complete_reconfigure();

    // we cannot call set_output_device in between begin/complete reconfigure
    // all the graph connections must be present
    for (size_t i = 0; i < demods.size(); ++i)
    {
        demods[i]->set_output_device(output_devstr, d_audio_rate); // update audio stream name
        qInfo() << "reciever remove_rx calls subrx" << i << "set_output_device";
    }

    return nextSub;
}

void receiver::remove_demodulator(demodulator::sptr demod)
{
    qInfo() << "receiver::remove_demodulator begin";

    begin_reconfigure();

    {
        auto rmidx = demod->get_idx();

        std::vector<demodulator::sptr> next;
        for (size_t i = 0; i < demods.size(); ++i)
        {
            if (demods[i]->get_idx() != rmidx)
            {
                next.push_back(demods[i]);
            }
        }
        demods.swap(next);
        next.clear();
        demod.reset();
    }

    for (size_t i = 0; i < demods.size(); ++i)
    {
        demods[i]->set_idx(i); // re-index
        demods[i]->set_output_device(output_devstr, d_audio_rate); // update audio stream name
        qInfo() << "reciever remove_rx calls subrx" << i << "set_demod";
        demods[i]->set_demod(demods[i]->get_demod(), true, subrxsrc, d_quad_rate, d_audio_rate);
        qInfo() << "reciever remove_rx calls subrx" << i << "set_demod done";
    }
    complete_reconfigure();

    qInfo() << "receiver::remove_demodulator done";
}

rx_status receiver::set_freq_corr(double ppm)
{
    src->set_freq_corr(ppm);

    return STATUS_OK;
}

/** Set new FFT size. */
void receiver::set_iq_fft_size(int newsize)
{
    iq_fft->set_fft_size(newsize);
}

void receiver::set_iq_fft_window(int window_type)
{
    iq_fft->set_window_type(window_type);
}

/** Get latest baseband FFT data. */
void receiver::get_iq_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize)
{
    iq_fft->get_fft_data(fftPoints, fftsize);
}

void receiver::begin_reconfigure()
{
    qInfo() << "receiver begin reconfigure";

    // tb->lock() seems to hang occasioanlly
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    // XXX somewhat duplicate operations in set_input_device
    tb->disconnect_all(); // !!! surely not! - makes us use force below!
    qInfo() << "**** begin_reconfigure disconnected all";

    connect_all();
    qInfo() << "begin_reconfigure connected all";
}

void receiver::complete_reconfigure()
{
    qInfo() << "\n\n" << gr::dot_graph(tb).c_str() << "\n\n";

    if (d_running)
        tb->start();

    qInfo() << "receiver completed reconfigure";
}

rx_status receiver::set_demod(const size_t idx, rx_demod demod, bool force)
{
    qInfo() << "reciever set_demod starts";

    rx_status ret = STATUS_OK;

    if (idx >= demods.size()) {
        return rx_status::STATUS_ERROR;
    }

    begin_reconfigure();

    for (size_t i = 0; i < demods.size(); ++i)
    {
        // TODO: do not force
        if (i == idx) {
            demods[i]->set_demod(demod, true, subrxsrc, d_quad_rate, d_audio_rate);
        } else {
            demods[i]->set_demod(demods[i]->get_demod(), true, subrxsrc, d_quad_rate, d_audio_rate);
        }
        qInfo() << "set_demod for subrx" << i;
    }

    complete_reconfigure();

    qInfo() << "set_demod done";

    return ret;
}

///**
// * @brief Start I/Q data recorder.
// * @param filename The filename where to record.
// */
//rx_status receiver::start_iq_recording(const std::string filename)
//{
//    rx_status status = STATUS_OK;

//    if (d_recording_iq) {
//        std::cout << __func__ << ": already recording" << std::endl;
//        return STATUS_ERROR;
//    }

//    try
//    {
//        iq_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename.c_str(), true);
//    }
//    catch (std::runtime_error &e)
//    {
//        std::cout << __func__ << ": couldn't open I/Q file" << std::endl;
//        return STATUS_ERROR;
//    }

//    tb->lock();
//    if (d_decim >= 2)
//        tb->connect(input_decim, 0, iq_sink, 0);
//    else
//        tb->connect(src, 0, iq_sink, 0);
//    d_recording_iq = true;
//    tb->unlock();

//    return status;
//}

///** Stop I/Q data recorder. */
//rx_status receiver::stop_iq_recording()
//{
//    if (!d_recording_iq) {
//        /* error: we are not recording */
//        return STATUS_ERROR;
//    }

//    tb->lock();
//    iq_sink->close();

//    if (d_decim >= 2)
//        tb->disconnect(input_decim, 0, iq_sink, 0);
//    else
//        tb->disconnect(src, 0, iq_sink, 0);

//    tb->unlock();
//    iq_sink.reset();
//    d_recording_iq = false;

//    return STATUS_OK;
//}

///**
// * @brief Seek to position in IQ file source.
// * @param pos Byte offset from the beginning of the file.
// */
//rx_status receiver::seek_iq_file(long pos)
//{
//    rx_status status = STATUS_OK;

//    tb->lock();

//    if (src->seek(pos, SEEK_SET))
//    {
//        status = STATUS_OK;
//    }
//    else
//    {
//        status = STATUS_ERROR;
//    }

//    tb->unlock();

//    return status;
//}

/** Convenience function to connect all blocks. */
void receiver::connect_all()
{
    qInfo() << "receiver connect_all starts";

    // Setup source
    subrxsrc = src;

    // Pre-processing
    if (d_decim >= 2)
    {
        tb->connect(subrxsrc, 0, input_decim, 0);
        subrxsrc = input_decim;
        qInfo() << "receiver connect_all using decim";
    }

//    if (d_recording_iq)
//    {
//        // We record IQ with minimal pre-processing
//        tb->connect(b, 0, iq_sink, 0);
//    }

    tb->connect(subrxsrc, 0, iq_swap, 0);
    subrxsrc = iq_swap;
    qInfo() << "receiver connect_all connected iq_swap";

    if (d_dc_cancel)
    {
        tb->connect(subrxsrc, 0, dc_corr, 0);
        subrxsrc = dc_corr;
        qInfo() << "receiver connect_all using dc cancel";
    }

    // Visualization
    tb->connect(subrxsrc, 0, iq_fft, 0);
    qInfo() << "receiver connect_all connected fft";
}
