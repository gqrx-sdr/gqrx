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

/* Tests for the TX helpers (device string, split offset, CTCSS table). */
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "dsp/tx_util.h"

static int failures = 0;

#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cerr << "FAIL: " << msg << " (" #cond ")" << std::endl;    \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void test_device_string()
{
    CHECK(tx_device_string("hackrf=0", "rtl=0") == "hackrf=0", "tx device wins");
    CHECK(tx_device_string("", "rtl=0") == "rtl=0", "fallback to input");
    CHECK(tx_device_string("", "").empty(), "nothing configured");
    CHECK(tx_device_string("", "file=/tmp/x.raw,freq=100e6").empty(),
          "file input cannot transmit");
    CHECK(tx_device_string("  uhd,type=b200 \n", "") == "uhd,type=b200", "trim tx");
    CHECK(tx_device_string("   ", "  hackrf=0 ") == "hackrf=0", "trim input");
    CHECK(tx_device_string("   ", "  file=x ").empty(), "trimmed file input");
}

static void test_frequency()
{
    CHECK(tx_frequency(145e6, 600e3) == 145.6e6, "positive split");
    CHECK(tx_frequency(145e6, -600e3) == 144.4e6, "negative split");
    CHECK(tx_frequency(145e6, 0.0) == 145e6, "no split");
    CHECK(tx_frequency(100e3, -200e3) == 0.0, "negative result");
    CHECK(tx_frequency(100e3, -100e3) == 0.0, "zero result");
}

static void test_ctcss()
{
    const std::vector<float> &t = ctcss_tones();

    CHECK(t.size() == 40, "tone count " << t.size());
    CHECK(t.front() == 67.0f, "first tone");
    CHECK(t.back() == 254.1f, "last tone");
    for (size_t i = 1; i < t.size(); i++)
        CHECK(t[i] > t[i - 1], "tones increase at index " << i);

    CHECK(ctcss_valid(100.0f), "100.0 is valid");
    CHECK(ctcss_valid(67.0f), "67.0 is valid");
    CHECK(!ctcss_valid(101.0f), "101.0 is not valid");
    CHECK(!ctcss_valid(0.0f), "0 is not valid");

    float voice, tone;
    ctcss_levels(0.15f, voice, tone);
    CHECK(std::abs(tone - 0.15f) < 1e-6, "tone level");
    CHECK(std::abs(voice + tone - 1.0f) < 1e-6, "levels sum to 1");

    ctcss_levels(0.0f, voice, tone);
    CHECK(std::abs(tone - 0.05f) < 1e-6, "clamp low");
    CHECK(std::abs(voice + tone - 1.0f) < 1e-6, "low sum");

    ctcss_levels(5.0f, voice, tone);
    CHECK(std::abs(tone - 0.3f) < 1e-6, "clamp high");
    CHECK(std::abs(voice + tone - 1.0f) < 1e-6, "high sum");
}

int main()
{
    test_device_string();
    test_frequency();
    test_ctcss();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX util checks passed" << std::endl;
    return EXIT_SUCCESS;
}
