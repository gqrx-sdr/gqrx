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

/* Tests for the TX telemetry provider and the text shown in the TX dock. */
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "dsp/tx_telemetry.h"

static int failures = 0;

#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cerr << "FAIL: " << msg << " (" #cond ")" << std::endl;    \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void test_null_provider()
{
    null_tx_telemetry_provider p;
    tx_meters m = p.read_meters();
    const std::string dash = tx_telemetry_dash();

    CHECK(dash == "\xe2\x80\x93", "dash is an en-dash");
    CHECK(!m.fwd_power && !m.alc && !m.swr, "null provider has no meters");
    CHECK(tx_telemetry_fwd_text(m.fwd_power) == dash, "fwd dash");
    CHECK(tx_telemetry_alc_text(m.alc) == dash, "alc dash");
    CHECK(tx_telemetry_swr_text(m.swr) == dash, "swr dash");
    CHECK(tx_telemetry_rate_text(false, 1.92e6) == dash, "rate dash");
    CHECK(tx_telemetry_freq_text(false, 145e6) == dash, "freq dash");
    CHECK(tx_telemetry_gain_text(false, 23.0, 0.0, 47.0) == dash, "gain dash");
}

static void test_fake_provider()
{
    tx_meters set;
    set.fwd_power = 10.0;
    set.alc = 2.5;
    set.swr = 1.50;

    fake_tx_telemetry_provider p(set);
    const tx_telemetry_provider &base = p;
    tx_meters m = base.read_meters();

    CHECK(m.fwd_power && *m.fwd_power == 10.0, "fwd value");
    CHECK(m.alc && *m.alc == 2.5, "alc value");
    CHECK(m.swr && *m.swr == 1.50, "swr value");
    CHECK(tx_telemetry_fwd_text(m.fwd_power) == "10.0 W", "fwd text");
    CHECK(tx_telemetry_alc_text(m.alc) == "2.5 dB", "alc text");
    CHECK(tx_telemetry_swr_text(m.swr) == "1.50", "swr text");
}

static void test_rate_freq_gain()
{
    CHECK(tx_telemetry_rate_text(true, 1920000.0) == "1.920 Msps", "rate");
    CHECK(tx_telemetry_freq_text(true, 145.6e6) == "145.600000 MHz", "freq");
    CHECK(tx_telemetry_gain_text(true, 23.0, 0.0, 47.0) ==
              "23.0 dB (0\xe2\x80\x93" "47)", "gain");
}

static void test_rigctl()
{
    /* ALC and SWR available, forward power rejected */
    tx_meters m = read_rigctl_meters([](const char *level, double &out) {
        if (!strcmp(level, "ALC")) { out = 0.5; return true; }
        if (!strcmp(level, "SWR")) { out = 1.2; return true; }
        return false;
    });
    CHECK(!m.fwd_power, "fwd power stays empty");
    CHECK(m.alc && *m.alc == 0.5, "rigctl alc");
    CHECK(m.swr && *m.swr == 1.2, "rigctl swr");

    /* RFPOWER is the fallback for RFPOWER_METER_WATTS */
    m = read_rigctl_meters([](const char *level, double &out) {
        if (!strcmp(level, "RFPOWER")) { out = 5.0; return true; }
        return false;
    });
    CHECK(m.fwd_power && *m.fwd_power == 5.0, "RFPOWER fallback");

    /* nothing available */
    m = read_rigctl_meters([](const char *, double &) { return false; });
    CHECK(!m.fwd_power && !m.alc && !m.swr, "all empty");
}

int main()
{
    test_null_provider();
    test_fake_provider();
    test_rate_freq_gain();
    test_rigctl();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX telemetry checks passed" << std::endl;
    return EXIT_SUCCESS;
}
