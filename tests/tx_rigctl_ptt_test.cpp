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

/* Tests for the hamlib PTT values used by the remote control. */
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

static void test_ptt()
{
    bool ok, enabled;

    tx_ptt_from_hamlib(0, ok, enabled);
    CHECK(ok && !enabled, "0 is RX");

    for (int v = 1; v <= 3; v++)
    {
        tx_ptt_from_hamlib(v, ok, enabled);
        CHECK(ok && enabled, "TX value " << v);
    }

    for (int v : {-1, 4, 100})
    {
        tx_ptt_from_hamlib(v, ok, enabled);
        CHECK(!ok && !enabled, "invalid value " << v);
    }
}

int main()
{
    test_ptt();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX rigctl PTT checks passed" << std::endl;
    return EXIT_SUCCESS;
}
