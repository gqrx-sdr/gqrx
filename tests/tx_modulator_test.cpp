/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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

/*
 * Runs the TX modulators in a flow graph (vector source -> modulator ->
 * vector sink) and checks the properties of the complex baseband output.
 */
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <gnuradio/blocks/vector_sink.h>
#include <gnuradio/blocks/vector_source.h>
#include <gnuradio/top_block.h>

#include "dsp/tx_modulator.h"

static const float  AUDIO_RATE = 48000.0f;
static const float  TONE_FREQ = 1000.0f;
static const size_t NUM_SAMPLES = 48000;
static const size_t SKIP = 8000;        /* skip filter transient */
static const float  MAX_DEV = 2500.0f;

static int failures = 0;

#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cerr << "FAIL: " << msg << " (" #cond ")" << std::endl;    \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static std::vector<gr_complex> run_modulator(tx_mod_mode mode, float amplitude,
                                             float ctcss_hz = 0.0f,
                                             int dcs_code = 0,
                                             bool dcs_inverted = false)
{
    std::vector<float> tone(NUM_SAMPLES);
    for (size_t i = 0; i < NUM_SAMPLES; i++)
        tone[i] = amplitude * std::sin(2.0 * M_PI * TONE_FREQ * i / AUDIO_RATE);

    gr::top_block_sptr tb = gr::make_top_block("tx_modulator_test");
    auto src = gr::blocks::vector_source_f::make(tone, false);
    auto mod = make_tx_modulator(mode, AUDIO_RATE, MAX_DEV, 0.8f, ctcss_hz,
                                 0.15f, dcs_code, dcs_inverted);
    auto snk = gr::blocks::vector_sink_c::make();

    tb->connect(src, 0, mod, 0);
    tb->connect(mod, 0, snk, 0);
    tb->run();

    return snk->data();
}

/* Power of the complex tone at the given frequency (Goertzel / single DFT bin) */
static double tone_power(const std::vector<gr_complex> &x, double freq)
{
    std::complex<double> acc(0.0, 0.0);
    size_t n = 0;

    for (size_t i = SKIP; i < x.size(); i++, n++)
    {
        double ph = -2.0 * M_PI * freq * i / AUDIO_RATE;
        acc += std::complex<double>(x[i].real(), x[i].imag()) *
               std::complex<double>(std::cos(ph), std::sin(ph));
    }

    return std::norm(acc) / ((double)n * (double)n);
}

static bool all_finite(const std::vector<gr_complex> &x)
{
    for (const auto &s : x)
        if (!std::isfinite(s.real()) || !std::isfinite(s.imag()))
            return false;

    return true;
}

static double max_magnitude(const std::vector<gr_complex> &x)
{
    double m = 0.0;
    for (const auto &s : x)
        m = std::max(m, (double)std::abs(s));

    return m;
}

static void test_am()
{
    auto x = run_modulator(TX_MOD_AM, 0.5f);

    CHECK(x.size() >= NUM_SAMPLES * 9 / 10, "AM: output length");
    CHECK(all_finite(x), "AM: finite output");
    CHECK(max_magnitude(x) <= 0.91, "AM: output level");

    double sum = 0.0, minv = 1e9, maxv = 0.0;
    size_t n = 0;
    for (size_t i = SKIP; i < x.size(); i++, n++)
    {
        double env = std::abs(x[i]);
        sum += env;
        minv = std::min(minv, env);
        maxv = std::max(maxv, env);
        CHECK(std::abs(x[i].imag()) < 1e-6, "AM: no quadrature component");
    }

    /* carrier level is 0.9 / 1.8 = 0.5, modulation depth 0.8 * 0.5 = 0.4 */
    CHECK(std::abs(sum / n - 0.5) < 0.02, "AM: mean envelope is the carrier");
    CHECK(maxv > 0.65 && maxv < 0.75, "AM: envelope peak");
    CHECK(minv > 0.25 && minv < 0.35, "AM: envelope valley");
}

static void test_nfm()
{
    const float amplitude = 0.8f;
    auto x = run_modulator(TX_MOD_NFM, amplitude);

    CHECK(x.size() >= NUM_SAMPLES * 9 / 10, "NFM: output length");
    CHECK(all_finite(x), "NFM: finite output");

    double peak_dev = 0.0;
    for (size_t i = SKIP; i + 1 < x.size(); i++)
    {
        CHECK(std::abs(std::abs(x[i]) - 1.0f) < 0.01, "NFM: constant envelope");
        double dph = std::arg(x[i + 1] * std::conj(x[i]));
        peak_dev = std::max(peak_dev, std::abs(dph) * AUDIO_RATE / (2.0 * M_PI));
    }

    double expected = amplitude * MAX_DEV;
    CHECK(std::abs(peak_dev - expected) < 0.1 * expected,
          "NFM: peak deviation " << peak_dev << " Hz, expected " << expected);
}

/* Instantaneous frequency in Hz between consecutive samples */
static std::vector<double> inst_freq(const std::vector<gr_complex> &x)
{
    std::vector<double> f;
    for (size_t i = SKIP; i + 1 < x.size(); i++)
        f.push_back(std::arg(x[i + 1] * std::conj(x[i])) * AUDIO_RATE / (2.0 * M_PI));

    return f;
}

static void test_nfm_ctcss()
{
    const float tone_hz = 100.0f;
    const float tone_level = 0.15f;

    /* silent audio with CTCSS: only the tone deviates the carrier */
    auto x = run_modulator(TX_MOD_NFM, 0.0f, tone_hz);
    CHECK(all_finite(x), "CTCSS: finite output");

    auto f = inst_freq(x);
    double peak = 0.0;
    for (double v : f)
        peak = std::max(peak, std::abs(v));

    double expected = tone_level * MAX_DEV;
    CHECK(std::abs(peak - expected) < 0.1 * expected,
          "CTCSS: peak deviation " << peak << " Hz, expected " << expected);

    /* tone period from the positive-going zero crossings */
    std::vector<size_t> crossings;
    for (size_t i = 1; i < f.size(); i++)
        if (f[i - 1] < 0.0 && f[i] >= 0.0)
            crossings.push_back(i);

    CHECK(crossings.size() > 10, "CTCSS: tone present");
    if (crossings.size() > 10)
    {
        double period = (double)(crossings.back() - crossings.front()) /
                        (crossings.size() - 1);
        double expected_period = AUDIO_RATE / tone_hz;
        CHECK(std::abs(period - expected_period) < 0.02 * expected_period,
              "CTCSS: tone period " << period << " samples, expected " << expected_period);
    }

    /* no tone requested: no deviation */
    auto y = run_modulator(TX_MOD_NFM, 0.0f, 0.0f);
    double peak_off = 0.0;
    for (double v : inst_freq(y))
        peak_off = std::max(peak_off, std::abs(v));
    CHECK(peak_off < 1.0, "CTCSS off: deviation " << peak_off << " Hz");

    /* loud voice plus tone must stay within the maximum deviation */
    auto z = run_modulator(TX_MOD_NFM, 1.0f, tone_hz);
    double peak_loud = 0.0;
    for (double v : inst_freq(z))
        peak_loud = std::max(peak_loud, std::abs(v));
    CHECK(peak_loud <= 1.1 * MAX_DEV,
          "CTCSS: loud voice deviation " << peak_loud << " Hz");
}

static void test_nfm_dcs()
{
    const float level = 0.15f;

    /* silent audio with DCS: only the code deviates the carrier */
    auto x = run_modulator(TX_MOD_NFM, 0.0f, 0.0f, 023, false);
    CHECK(all_finite(x), "DCS: finite output");

    auto f = inst_freq(x);
    double peak = 0.0;
    for (double v : f)
        peak = std::max(peak, std::abs(v));

    double expected = level * MAX_DEV;
    CHECK(std::abs(peak - expected) < 0.1 * expected,
          "DCS: peak deviation " << peak << " Hz, expected " << expected);

    /* the shortest run of equal sign is one bit */
    std::vector<size_t> crossings;
    for (size_t i = 1; i < f.size(); i++)
        if ((f[i - 1] < 0.0) != (f[i] < 0.0))
            crossings.push_back(i);

    CHECK(crossings.size() > 10, "DCS: transitions present");
    double shortest = 1e9;
    for (size_t i = 1; i < crossings.size(); i++)
        shortest = std::min(shortest, (double)(crossings[i] - crossings[i - 1]));
    double bit = AUDIO_RATE / 134.4;
    CHECK(std::abs(shortest - bit) < 0.02 * bit,
          "DCS: bit period " << shortest << " samples, expected " << bit);

    /* no code requested: no deviation */
    auto y = run_modulator(TX_MOD_NFM, 0.0f, 0.0f, 0, false);
    double peak_off = 0.0;
    for (double v : inst_freq(y))
        peak_off = std::max(peak_off, std::abs(v));
    CHECK(peak_off < 1.0, "DCS off: deviation " << peak_off << " Hz");

    /* loud voice plus DCS must stay within the maximum deviation */
    auto z = run_modulator(TX_MOD_NFM, 1.0f, 0.0f, 023, true);
    double peak_loud = 0.0;
    for (double v : inst_freq(z))
        peak_loud = std::max(peak_loud, std::abs(v));
    CHECK(peak_loud <= 1.1 * MAX_DEV,
          "DCS: loud voice deviation " << peak_loud << " Hz");
}

static void test_ssb(tx_mod_mode mode)
{
    const char *name = (mode == TX_MOD_USB) ? "USB" : "LSB";
    auto x = run_modulator(mode, 0.5f);

    CHECK(x.size() >= NUM_SAMPLES * 9 / 10, name << ": output length");
    CHECK(all_finite(x), name << ": finite output");
    CHECK(max_magnitude(x) <= 0.91, name << ": output level");

    double wanted = tone_power(x, (mode == TX_MOD_USB) ? TONE_FREQ : -TONE_FREQ);
    double unwanted = tone_power(x, (mode == TX_MOD_USB) ? -TONE_FREQ : TONE_FREQ);

    CHECK(wanted > 1e-3, name << ": wanted sideband present");
    CHECK(10.0 * std::log10(wanted / (unwanted + 1e-30)) > 20.0,
          name << ": unwanted sideband suppression");
}

int main()
{
    test_am();
    test_nfm();
    test_nfm_ctcss();
    test_nfm_dcs();
    test_ssb(TX_MOD_USB);
    test_ssb(TX_MOD_LSB);

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX modulator checks passed" << std::endl;
    return EXIT_SUCCESS;
}
