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

/* Tests for the TX power and microphone gain mapping. */
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

static void test_power()
{
    CHECK(tx_power_gain(0, 0.0, 47.0) == 0.0, "0 % is min");
    CHECK(tx_power_gain(100, 0.0, 47.0) == 47.0, "100 % is max");
    CHECK(std::abs(tx_power_gain(50, 0.0, 47.0) - 23.5) < 1e-9, "50 %");
    CHECK(tx_power_gain(-10, 0.0, 47.0) == 0.0, "clamp low");
    CHECK(tx_power_gain(250, 0.0, 47.0) == 47.0, "clamp high");
    CHECK(tx_power_gain(50, 10.0, 10.0) == 10.0, "empty range");
    CHECK(tx_power_gain(50, 10.0, 5.0) == 10.0, "inverted range");
}

static void test_mic()
{
    CHECK(std::abs(tx_mic_linear_gain(0.0f) - 1.0f) < 1e-6, "0 dB");
    CHECK(std::abs(tx_mic_linear_gain(20.0f) - 10.0f) < 1e-4, "+20 dB");
    CHECK(std::abs(tx_mic_linear_gain(-20.0f) - 0.1f) < 1e-6, "-20 dB");
}

int main()
{
    test_power();
    test_mic();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX power and mic gain checks passed" << std::endl;
    return EXIT_SUCCESS;
}
