/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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

/* Offline tests for the transmit modulators. These tests do not
 * require any SDR hardware: the audio input is generated with a
 * vector source and the modulated output is captured with a vector
 * sink.
 */

#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

#include <gnuradio/blocks/head.h>
#include <gnuradio/blocks/vector_sink_c.h>
#include <gnuradio/blocks/vector_source_f.h>
#include <gnuradio/top_block.h>

#include "dsp/tx_mod_am.h"
#include "dsp/tx_mod_nfm.h"
#include "dsp/tx_mod_ssb.h"

#define AUDIO_RATE 48000.0f
#define SAMPLES    48000  /* one second */

/* Number of samples skipped at the beginning to account for the
 * settling time of the audio filters. */
#define SKIP       4800

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { \
            std::printf("PASS: %s\n", msg); \
        } else { \
            std::printf("FAIL: %s\n", msg); \
            failures++; \
        } \
    } while (0)

/* Generate one second of a sine wave. */
static std::vector<float> sine(float freq, float amp)
{
    std::vector<float> data(SAMPLES);

    for (int i = 0; i < SAMPLES; i++)
    {
        data[i] = amp * sinf(2.0f * (float)M_PI * freq * i / AUDIO_RATE);
    }

    return data;
}

/* Run samples through a float-input modulator and return the output. */
static std::vector<gr_complex> run_float(gr::hier_block2::sptr mod,
                                         const std::vector<float> &input)
{
    auto tb = gr::make_top_block("tx_test");

    auto src = gr::blocks::vector_source_f::make(input);
    auto head = gr::blocks::head::make(sizeof(gr_complex), input.size());
    auto sink = gr::blocks::vector_sink_c::make();

    tb->connect(src, 0, mod, 0);
    tb->connect(mod, 0, head, 0);
    tb->connect(head, 0, sink, 0);
    tb->run();

    return sink->data();
}

/* NFM: constant envelope, correct deviation, limited when overdriven. */
static void test_nfm(void)
{
    auto mod = make_tx_mod_nfm(AUDIO_RATE, 5000.0f);

    /* 1 kHz tone at 0.5 amplitude -> 2500 Hz deviation */
    auto out = run_float(mod, sine(1000.0f, 0.5f));

    /* constant envelope */
    float mag_min = 2.0f, mag_max = 0.0f;
    for (unsigned int i = SKIP; i < out.size(); i++)
    {
        float mag = std::abs(out[i]);
        if (mag < mag_min) mag_min = mag;
        if (mag > mag_max) mag_max = mag;
    }
    CHECK(mag_min > 0.9f && mag_max < 1.1f, "NFM constant envelope");

    /* deviation */
    float dev_max = 0.0f;
    for (unsigned int i = SKIP + 1; i < out.size(); i++)
    {
        gr_complex d = out[i] * std::conj(out[i - 1]);
        float freq = std::arg(d) * AUDIO_RATE / (2.0f * (float)M_PI);
        if (freq > dev_max) dev_max = freq;
    }
    CHECK(dev_max > 2000.0f && dev_max < 3000.0f, "NFM deviation of 0.5 amp tone");

    /* overdriven input must not exceed max_dev */
    auto out2 = run_float(make_tx_mod_nfm(AUDIO_RATE, 5000.0f),
                          sine(1000.0f, 5.0f));
    float dev_max2 = 0.0f;
    for (unsigned int i = SKIP + 1; i < out2.size(); i++)
    {
        gr_complex d = out2[i] * std::conj(out2[i - 1]);
        float freq = std::arg(d) * AUDIO_RATE / (2.0f * (float)M_PI);
        if (freq > dev_max2) dev_max2 = freq;
    }
    CHECK(dev_max2 <= 5100.0f, "NFM deviation limited when overdriven");
}

/* AM: envelope = carrier + modulated audio, real valued. */
static void test_am(void)
{
    auto mod = make_tx_mod_am(AUDIO_RATE, 0.5f);
    auto out = run_float(mod, sine(1000.0f, 1.0f));

    float re_max = -10.0f, re_min = 10.0f, im_max = 0.0f;
    float re_mean = 0.0f;
    int count = 0;

    for (unsigned int i = SKIP; i < out.size(); i++)
    {
        float re = out[i].real();
        float im = out[i].imag();
        if (re > re_max) re_max = re;
        if (re < re_min) re_min = re;
        if (fabsf(im) > im_max) im_max = fabsf(im);
        re_mean += re;
        count++;
    }
    re_mean /= count;

    /* carrier at 1.0, envelope between 0 and 2 (m=1, amp=1) */
    CHECK(re_mean > 0.8f && re_mean < 1.2f, "AM carrier level");
    CHECK(re_max > 1.6f && re_max < 2.4f, "AM envelope peak");
    CHECK(re_min > -0.2f && re_min < 0.4f, "AM envelope minimum");
    CHECK(im_max < 0.15f, "AM is real valued");
}

/* SSB: USB keeps positive frequencies, LSB the negative ones. */
static void test_ssb(void)
{
    auto usb = make_tx_mod_ssb(AUDIO_RATE, true);
    auto out = run_float(usb, sine(1000.0f, 1.0f));

    /* For a tone at f the analytic signal is exp(+j2*pi*f*t) */
    float err_max = 0.0f;
    for (unsigned int i = SKIP; i < out.size(); i++)
    {
        float t = (float)i / AUDIO_RATE;
        gr_complex expected(cosf(2.0f * (float)M_PI * 1000.0f * t),
                            sinf(2.0f * (float)M_PI * 1000.0f * t));
        float err = std::abs(out[i] - expected);
        if (err > err_max) err_max = err;
    }
    CHECK(err_max < 0.4f, "USB analytic signal");

    auto lsb = make_tx_mod_ssb(AUDIO_RATE, false);
    auto out2 = run_float(lsb, sine(1000.0f, 1.0f));

    /* For LSB the tone is at -f */
    float err_max2 = 0.0f;
    for (unsigned int i = SKIP; i < out2.size(); i++)
    {
        float t = (float)i / AUDIO_RATE;
        gr_complex expected(cosf(2.0f * (float)M_PI * 1000.0f * t),
                           -sinf(2.0f * (float)M_PI * 1000.0f * t));
        float err = std::abs(out2[i] - expected);
        if (err > err_max2) err_max2 = err;
    }
    CHECK(err_max2 < 0.4f, "LSB conjugate signal");
}

/* CTCSS: with silence input and CTCSS enabled, the modulator must
 * produce frequency modulation at the tone frequency. */
static void test_ctcss(void)
{
    auto mod = make_tx_mod_nfm(AUDIO_RATE, 5000.0f);
    mod->set_ctcss(88.5f);

    auto out = run_float(mod, std::vector<float>(SAMPLES, 0.0f));

    /* measure peak instantaneous frequency */
    float dev_max = 0.0f;
    for (unsigned int i = SKIP + 1; i < out.size(); i++)
    {
        gr_complex d = out[i] * std::conj(out[i - 1]);
        float freq = std::arg(d) * AUDIO_RATE / (2.0f * (float)M_PI);
        if (freq > dev_max) dev_max = freq;
    }

    /* tone deviation is 15% of max_dev = 750 Hz */
    CHECK(dev_max > 500.0f && dev_max < 1000.0f, "CTCSS tone deviation");

    /* disabled again */
    auto mod2 = make_tx_mod_nfm(AUDIO_RATE, 5000.0f);
    mod2->set_ctcss(0.0f);
    auto out2 = run_float(mod2, std::vector<float>(SAMPLES, 0.0f));
    float dev_max2 = 0.0f;
    for (unsigned int i = SKIP + 1; i < out2.size(); i++)
    {
        gr_complex d = out2[i] * std::conj(out2[i - 1]);
        float freq = std::arg(d) * AUDIO_RATE / (2.0f * (float)M_PI);
        if (freq > dev_max2) dev_max2 = freq;
    }
    CHECK(dev_max2 < 50.0f, "CTCSS disabled produces no tone");
}

int main(void)
{
    std::printf("Gqrx TX modulator tests\n");

    test_nfm();
    test_am();
    test_ssb();
    test_ctcss();

    if (failures)
    {
        std::printf("%d test(s) FAILED\n", failures);
        return 1;
    }

    std::printf("All tests passed\n");
    return 0;
}
