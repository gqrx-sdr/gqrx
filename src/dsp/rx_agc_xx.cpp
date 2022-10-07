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
#include <math.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <dsp/rx_agc_xx.h>
#include <volk/volk.h>

#define NO_AGC_DEBUG

#define exp10f(K) powf(10.0,(K))
#define exp10(K) pow(10.0,(K))

#define MIN_GAIN_DB (-20.0f)
#define MIN_GAIN exp10f(MIN_GAIN_DB)
#define MAX_SAMPLE_RATE 96000
#define PANNING_DELAY_K 4000.0
#define PANNING_GAIN_K  100.0
//TODO Make this user-configurable as extra peak history time
#define AGC_AVG_BUF_SCALE 2

rx_agc_2f_sptr make_rx_agc_2f(double sample_rate, bool agc_on, int target_level,
                              int manual_gain, int max_gain, int attack,
                              int decay, int hang, int panning)
{
    return gnuradio::get_initial_sptr(new rx_agc_2f(sample_rate, agc_on, target_level,
                                                    manual_gain, max_gain, attack, decay,
                                                    hang, panning));
}

/**
 * \brief Create receiver AGC object.
 *
 * Use make_rx_agc_2f() instead.
 */
rx_agc_2f::rx_agc_2f(double sample_rate, bool agc_on, int target_level,
                              int manual_gain, int max_gain, int attack,
                              int decay, int hang, int panning)
    : gr::sync_block ("rx_agc_2f",
          gr::io_signature::make(2, 2, sizeof(float)),
          gr::io_signature::make(4, 4, sizeof(float))),
      d_agc_on(agc_on),
      d_sample_rate(sample_rate),
      d_target_level(target_level),
      d_manual_gain(manual_gain),
      d_max_gain(max_gain),
      d_attack(attack),
      d_decay(decay),
      d_hang(hang),
      d_panning(panning),
      d_mute(false),
      d_target_mag(1),
      d_hang_samp(0),
      d_buf_samples(0),
      d_buf_size(0),
      d_max_idx(0),
      d_buf_p(0),
      d_hang_counter(0),
      d_max_gain_mag(1.0),
      d_current_gain(1.0),
      d_target_gain(1.0),
      d_decay_step(1.01),
      d_attack_step(0.99),
      d_floor(0.0001),
      d_gain_l(1.0),
      d_gain_r(1.0),
      d_delay_l(0),
      d_delay_r(0),
      d_refill(false),
      d_running(false)

{
    set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang, d_panning, true);
    set_history(MAX_SAMPLE_RATE + 1 + MAX_SAMPLE_RATE * 100 / PANNING_DELAY_K);
    set_tag_propagation_policy(TPP_DONT);
}

rx_agc_2f::~rx_agc_2f()
{
}

bool rx_agc_2f::start()
{
    d_running = true;
    return gr::sync_block::start();
}

bool rx_agc_2f::stop()
{
    d_running = false;
    return gr::sync_block::stop();
}

/**
 * \brief Receiver AGC work method.
 * \param mooutput_items
 * \param input_items
 * \param output_items
 */
int rx_agc_2f::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    const float *in0 = (const float *) input_items[0];
    const float *in1 = (const float *) input_items[1];
    float *out0 = (float *) output_items[0];
    float *out1 = (float *) output_items[1];
    float *out2 = (float *) output_items[2];
    float *out3 = (float *) output_items[3];

    std::lock_guard<std::mutex> lock(d_mutex);

    int k;
    TYPEFLOAT max_out = 0;
    TYPEFLOAT mag_in = 0;
    if (d_agc_on)
    {
        std::vector<gr::tag_t> work_tags;
        get_tags_in_window(work_tags, 0, 0, noutput_items);
        for (const auto& tag : work_tags)
            add_item_tag(0, tag.offset + d_buf_samples, tag.key, tag.value);
        get_tags_in_window(work_tags, 1, 0, noutput_items);
        for (const auto& tag : work_tags)
            add_item_tag(1, tag.offset + d_buf_samples, tag.key, tag.value);
        if (d_refill)
        {
            d_refill = false;
            int p = history() - 1 - d_buf_size;
            for (k = 0; k < d_buf_size; k++, p++)
            {
                float sample_in0 = in0[p];
                float sample_in1 = in1[p];
                mag_in = std::max(fabs(sample_in0),fabs(sample_in1));
                d_mag_buf[k] = mag_in;
                update_buffer(k);
            }
        }
        for (k = 0; k < noutput_items; k++)
        {
            int k_hist = k + history() - 1;
            float sample_in0 = in0[k_hist];
            float sample_in1 = in1[k_hist];
            mag_in = std::max(fabs(sample_in0),fabs(sample_in1));
            float sample_out0 = in0[k_hist - d_buf_samples];
            float sample_out1 = in1[k_hist - d_buf_samples];

            d_mag_buf[d_buf_p] = mag_in;
            update_buffer(d_buf_p);
            max_out = get_peak();

            int buf_p_next = d_buf_p + 1;
            if (buf_p_next >= d_buf_size)
                buf_p_next = 0;

            if (max_out > d_floor)
            {
                float new_target = d_target_mag / max_out;
                if (new_target < d_target_gain)
                {
                    if (d_current_gain > d_target_gain)
                        d_hang_counter = d_buf_samples + d_hang_samp;
                    d_target_gain = new_target;
                }
                else
                    if (!d_hang_counter)
                        d_target_gain = new_target;
            }
            else
            {
                d_target_gain = d_max_gain_mag;
                d_hang_counter = 0;
            }
            if (d_current_gain > d_target_gain)
            {
                //attack, decrease gain one step per sample
                d_current_gain *= d_attack_step;
            }
            else
            {
                if (d_hang_counter <= 0)
                {
                    //decay, increase gain one step per sample until we reach d_max_gain
                    if (d_current_gain < d_target_gain)
                        d_current_gain *= d_decay_step;
                    if (d_current_gain > d_target_gain)
                        d_current_gain = d_target_gain;
                }
            }
            if (d_hang_counter > 0)
                d_hang_counter--;
            if (d_current_gain < MIN_GAIN)
                d_current_gain = MIN_GAIN;
            if (d_mute)
            {
                out2[k] = 0.0;
                out3[k] = 0.0;
            }else{
                out2[k] = in0[k_hist - d_buf_samples - d_delay_l] * d_current_gain * d_gain_l;
                out3[k] = in1[k_hist - d_buf_samples - d_delay_r] * d_current_gain * d_gain_r;
            }
            out0[k] = sample_out0 * d_current_gain;
            out1[k] = sample_out1 * d_current_gain;
            d_buf_p = buf_p_next;
        }
    }
    else{
        std::vector<gr::tag_t> work_tags;
        get_tags_in_window(work_tags, 0, 0, noutput_items);
        for (const auto& tag : work_tags)
            add_item_tag(0, tag.offset, tag.key, tag.value);
        get_tags_in_window(work_tags, 1, 0, noutput_items);
        for (const auto& tag : work_tags)
            add_item_tag(1, tag.offset, tag.key, tag.value);
        if (d_mute)
        {
            std::memset(out2, 0, sizeof(float) * noutput_items);
            std::memset(out3, 0, sizeof(float) * noutput_items);
        }else{
            volk_32f_s32f_multiply_32f((float *)out2, (float *)&in0[history() - 1 - d_delay_l], d_current_gain * d_gain_l, noutput_items);
            volk_32f_s32f_multiply_32f((float *)out3, (float *)&in1[history() - 1 - d_delay_r], d_current_gain * d_gain_r, noutput_items);
        }
        volk_32f_s32f_multiply_32f((float *)out0, (float *)&in0[history() - 1], d_current_gain, noutput_items);
        volk_32f_s32f_multiply_32f((float *)out1, (float *)&in1[history() - 1], d_current_gain, noutput_items);
    }
    #ifdef AGC_DEBUG2
    static TYPEFLOAT d_prev_dbg = 0.0;
    if(d_prev_dbg != d_target_gain)
    {
        std::cerr<<"------ d_target_gain="<<d_target_gain<<" d_current_gain="<<d_current_gain<<" d_hang_counter="<<d_hang_counter<<  std::endl;
        d_prev_dbg = d_target_gain;
    }
    #endif

    return noutput_items;
}

/**
 * \brief Enable or disable AGC.
 * \param agc_on Whether AGC should be endabled.
 *
 * When AGC is disabled a fixed gain is used.
 *
 * \sa set_manual_gain()
 */
void rx_agc_2f::set_agc_on(bool agc_on)
{
    if (agc_on != d_agc_on) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set AGC sample rate.
 * \param sample_rate The sample rate.
 *
 * The AGC uses knowledge about the sample rate to calculate various delays and
 * time constants.
 */
void rx_agc_2f::set_sample_rate(double sample_rate)
{
    if (sample_rate != d_sample_rate) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set new AGC target level.
 * \param threshold The new target level between -160 and 0dB.
 *
 * Maximum output signal lenvel in dB..
 */
void rx_agc_2f::set_target_level(int target_level)
{
    if ((target_level != d_target_level) && (target_level >= -160) && (target_level <= 0)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set new manual gain.
 * \param gain The new manual gain between -160 and 160dB.
 *
 * The manual gain is used when AGC is switched off.
 *
 * \sa set_agc_on()
 */
void rx_agc_2f::set_manual_gain(float gain)
{
    if(d_agc_on)
    {
        d_manual_gain = gain;
        return;
    }
    if ((gain != d_manual_gain) && (gain >= -160.f) && (gain <= 160.f)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, gain, d_max_gain, d_attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set new max gain.
 * \param gain The new max gain between 0 and 100dB.
 *
 * Limits maximum AGC gain to reduce noise.
 *
 * \sa set_agc_on()
 */
void rx_agc_2f::set_max_gain(int gain)
{
    if ((gain != d_max_gain) && (gain >= 0) && (gain <= 160)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, gain, d_attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set AGC attack time.
 * \param decay The new AGC attack time between 20 to 5000 ms.
 *
 * Sets length of the delay buffer
 *
 */
void rx_agc_2f::set_attack(int attack)
{
    if ((attack != d_attack) && (attack >= 20) && (attack <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, attack, d_decay, d_hang, d_panning);
    }
}

/**
 * \brief Set AGC decay time.
 * \param decay The new AGC decay time between 20 to 5000 ms.
 */
void rx_agc_2f::set_decay(int decay)
{
    if ((decay != d_decay) && (decay >= 20) && (decay <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, decay, d_hang, d_panning);
    }
}

/**
 * \brief Set AGC hang time between 0 to 5000 ms.
 * \param hang Time to keep AGC gain at constant level after the peak.
 */
void rx_agc_2f::set_hang(int hang)
{
    if ((hang != d_hang) && (hang >= 0) && (hang <= 5000)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, hang, d_panning);
    }
}

void rx_agc_2f::set_panning(int panning)
{
    if((panning != d_panning) && (panning >=-100) && (panning <= 100)) {
        std::lock_guard<std::mutex> lock(d_mutex);
        set_parameters(d_sample_rate, d_agc_on, d_target_level, d_manual_gain, d_max_gain, d_attack, d_decay, d_hang, panning);
    }
}

/**
 * \brief Set mute enabled or disabled (audio output only).
 * \param mute New mute state.
 */
void rx_agc_2f::set_mute(bool mute)
{
    if (mute != d_mute) {
        std::lock_guard<std::mutex> lock(d_mutex);
        d_mute = mute;
    }
}

float rx_agc_2f::get_current_gain()
{
    std::lock_guard<std::mutex> lock(d_mutex);
    return 20.f * log10f(d_current_gain);
}

void rx_agc_2f::set_parameters(double sample_rate, bool agc_on, int target_level,
                              float manual_gain, int max_gain, int attack,
                              int decay, int hang, int panning, bool force)
{
    bool samp_rate_changed = false;
    bool agc_on_changed = false;
    bool target_level_changed = false;
    bool manual_gain_changed = false;
    bool max_gain_changed = false;
    bool attack_changed = false;
    bool decay_changed = false;
    bool hang_changed = false;

    if (d_sample_rate != sample_rate || force)
    {
        d_sample_rate = sample_rate;
        samp_rate_changed = true;
    }
    if (d_agc_on != agc_on  || force)
    {
        d_agc_on = agc_on;
        agc_on_changed = true;
        if(d_agc_on && d_running)
            d_refill = true;
    }
    if (d_target_level != target_level || force)
    {
        d_target_level = target_level;
        d_target_mag = exp10f(TYPEFLOAT(d_target_level) / 20.f) * 32767.f / 32768.f;
        target_level_changed = true;
    }
    if (d_manual_gain != manual_gain || force)
    {
        d_manual_gain = manual_gain;
        manual_gain_changed = true;
    }
    if (d_max_gain != max_gain || force)
    {
        d_max_gain = max_gain;
        if(d_max_gain < 1)
            d_max_gain = 1;
        d_max_gain_mag = exp10f(TYPEFLOAT(d_max_gain) / 20.f);
        max_gain_changed = true;
    }
    if (d_attack != attack || force)
    {
        d_attack = attack;
        attack_changed = true;
    }
    if (d_decay != decay || force)
    {
        d_decay = decay;
        decay_changed = true;
    }
    if (d_hang != hang || force)
    {
        d_hang = hang;
        hang_changed = true;
    }
    if (d_panning != panning || force)
    {
        d_panning = panning;
        d_gain_l = 1.0;
        d_gain_r = 1.0;
        d_delay_l = 0;
        d_delay_r = 0;
        if(panning < 0)
        {
            d_gain_r = exp10(panning / PANNING_GAIN_K);
            d_delay_r = (sample_rate * -panning)/PANNING_DELAY_K;
        }
        if(panning > 0)
        {
            d_gain_l = exp10(-panning / PANNING_GAIN_K);
            d_delay_l = (sample_rate * panning)/PANNING_DELAY_K;
        }
    }
    if (samp_rate_changed || attack_changed)
    {
        d_buf_samples = sample_rate * d_attack / 1000.0;
        int buf_samples = d_buf_samples * AGC_AVG_BUF_SCALE;
        int buf_size = 1;
        for(unsigned int k = 0; k < sizeof(int) * 8; k++)
        {
            buf_size *= 2;
            if(buf_size >= buf_samples)
                break;
        }
        if(d_buf_size != buf_size)
        {
            d_buf_size = buf_size;
            d_mag_buf.clear();
            d_mag_buf.resize(d_buf_size * 2, 0);
            d_buf_p = 0;
            d_max_idx = d_buf_size * 2 - 2;
            if(d_agc_on && d_running)
                d_refill = true;
         }
        if (d_buf_p >= d_buf_size)
            d_buf_p %= d_buf_size;
    }
    if ((manual_gain_changed || agc_on_changed) && !agc_on)
        d_current_gain = exp10f(TYPEFLOAT(d_manual_gain) / 20.f);
    if (max_gain_changed || attack_changed || samp_rate_changed)
        d_attack_step = 1.f / exp10f(std::max(TYPEFLOAT(d_max_gain), - MIN_GAIN_DB) / TYPEFLOAT(d_buf_samples) / 20.f);
    if (max_gain_changed || decay_changed || samp_rate_changed)
        d_decay_step = exp10f(TYPEFLOAT(d_max_gain) / TYPEFLOAT(sample_rate * d_decay / 1000.0) / 20.f);
    if (hang_changed || samp_rate_changed)
        d_hang_samp = sample_rate * d_hang / 1000.0;

    if (target_level_changed || max_gain_changed)
        d_floor = exp10f(TYPEFLOAT(d_target_level - d_max_gain) / 20.f);
    #ifdef AGC_DEBUG
    std::cerr<<std::endl<<
        "d_target_mag="<<d_target_mag<<std::endl<<
        "d_target_level="<<d_target_level<<std::endl<<
        "d_manual_gain="<<d_manual_gain<<std::endl<<
        "d_max_gain="<<d_max_gain<<std::endl<<
        "d_attack="<<d_attack<<std::endl<<
        "d_attack_step="<<log10(d_attack_step)*20.0<<std::endl<<
        "d_decay="<<d_decay<<std::endl<<
        "d_decay_step="<<log10(d_decay_step)*20.0<<std::endl<<
        "d_hang="<<d_hang<<std::endl<<
        "d_hang_samp="<<d_hang_samp<<std::endl<<
        "d_hang_counter="<<d_hang_counter<<std::endl<<
        "d_buf_samples="<<d_buf_samples<<std::endl<<
        "d_buf_size="<<d_buf_size<<std::endl<<
        "d_current_gain="<<d_current_gain<<std::endl<<
        "";
    #endif
}

float rx_agc_2f::get_peak()
{
    return d_mag_buf[d_max_idx];
}

void rx_agc_2f::update_buffer(int p)
{
    int ofs = 0;
    int base = d_buf_size;
    while (base > 1)
    {
        float max_p = std::max(d_mag_buf[ofs + p], d_mag_buf[ofs + (p ^ 1)]);
        p = p >> 1;
        ofs += base;
        if(d_mag_buf[ofs + p] != max_p)
            d_mag_buf[ofs + p] = max_p;
        else
            break;
        base = base >> 1;
    }

}
