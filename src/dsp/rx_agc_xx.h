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
#ifndef RX_AGC_XX_H
#define RX_AGC_XX_H

#include <mutex>
#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>

#define TYPECPX std::complex<float>
#define TYPEFLOAT float

class rx_agc_2f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_agc_2f> rx_agc_2f_sptr;
#else
typedef std::shared_ptr<rx_agc_2f> rx_agc_2f_sptr;
#endif


/**
 * \brief Return a shared_ptr to a new instance of rx_agc_cc.
 * \param sample_rate  The sample rate (default = 96000).
 * \param agc_on       Whether AGC should be ON (default = true).
 * \param target_level Target output level in dB if AGC is active. Range -160 to 0 dB.
 * \param manual_gain  Manual gain when AGC is OFF. Range -160 to 160 dB.
 * \param max_gain     Maximum gain when AGC is ON. Range 0 to 100 dB.
 * \param attack       AGC maximum attack time in milliseconds. Range 20 to 5000. This
 *                     parameter determines whether AGC is fast, slow or medium.
 *                     It is recommenfded to set it below 1000 ms to reduce audio lag.
 * \param decay        AGC decay time in milliseconds. Range 20 to 5000. This
 *                     parameter determines whether AGC is fast, slow or medium.
 * \param hang         The time AGC should "hang" before starting to decay in
 *                     milliseconds. Range 0 to 5000.
 *
 * This is effectively the public constructor for a new AGC block.
 * To avoid accidental use of raw pointers, the rx_agc_cc constructor is private.
 * make_rx_agc_cc is the public interface for creating new instances.
 */
rx_agc_2f_sptr make_rx_agc_2f(double sample_rate, bool agc_on, int target_level,
                              int manual_gain, int max_gain, int attack,
                              int decay, int hang, int panning);

/**
 * \brief Experimental AGC block for analog voice modes (AM, SSB, CW).
 * \ingroup DSP
 *
 * This block performs automatic gain control.
 * To be written...
 */
class rx_agc_2f : public gr::sync_block
{
    friend rx_agc_2f_sptr make_rx_agc_2f(double sample_rate, bool agc_on,
                                         int target_level, int manual_gain,
                                         int max_gain, int attack, int decay,
                                         int hang, int panning);

protected:
    rx_agc_2f(double sample_rate, bool agc_on, int target_level,
              int manual_gain, int max_gain, int attack, int decay, int hang,
              int panning);

public:
    ~rx_agc_2f();

    bool start() override;
    bool stop() override;
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    void set_agc_on(bool agc_on);
    void set_sample_rate(double sample_rate);
    void set_target_level(int target_level);
    void set_manual_gain(float gain);
    void set_max_gain(int gain);
    void set_attack(int attack);
    void set_decay(int decay);
    void set_hang(int hang);
    void set_panning(int panning);
    void set_mute(bool mute);

    float get_current_gain();
private:
    void set_parameters(double sample_rate, bool agc_on, int target_level,
                       float manual_gain, int max_gain, int attack, int decay,
                       int hang, int panning, bool force = false);

    std::mutex      d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    bool            d_agc_on;        /*! Current AGC status (true/false). */
    double          d_sample_rate;   /*! Current sample rate. */
    int             d_target_level;  /*! SGC target level (-160...0 dB). */
    float           d_manual_gain;   /*! Current gain when AGC is OFF. */
    int             d_max_gain;      /*! Maximum gain when AGC is ON. */
    int             d_attack;        /*! Current AGC attack (20...5000 ms). */
    int             d_decay;         /*! Current AGC decay (20...5000 ms). */
    int             d_hang;          /*! Current AGC hang (0...5000 ms). */
    int             d_panning;       /*! Current AGC panning (-100...100). */
    int             d_mute;          /*! Current AGC mute state. */
private:
    float get_peak();
    void update_buffer(int p);

    TYPEFLOAT d_target_mag;
    int d_hang_samp;
    int d_buf_samples;
    int d_buf_size;
    int d_max_idx;
    int d_buf_p;
    int d_hang_counter;
    TYPEFLOAT d_max_gain_mag;
    TYPEFLOAT d_current_gain;
    TYPEFLOAT d_target_gain;
    TYPEFLOAT d_decay_step;
    TYPEFLOAT d_attack_step;
    TYPEFLOAT d_floor;
    TYPEFLOAT d_gain_l;
    TYPEFLOAT d_gain_r;
    int d_delay_l;
    int d_delay_r;

    std::vector<float>   d_mag_buf;
    bool d_refill;
    bool d_running;
};

#endif /* RX_AGC_XX_H */
