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

static std::vector<gr_complex> run_modulator(tx_mod_mode mode, float amplitude)
{
    std::vector<float> tone(NUM_SAMPLES);
    for (size_t i = 0; i < NUM_SAMPLES; i++)
        tone[i] = amplitude * std::sin(2.0 * M_PI * TONE_FREQ * i / AUDIO_RATE);

    gr::top_block_sptr tb = gr::make_top_block("tx_modulator_test");
    auto src = gr::blocks::vector_source_f::make(tone, false);
    auto mod = make_tx_modulator(mode, AUDIO_RATE, MAX_DEV, 0.8f);
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
