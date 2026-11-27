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

/* Tests for the DCS table and word. */
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

static void test_dcs()
{
    const std::vector<int> &c = dcs_codes();

    CHECK(c.size() == 104, "code count " << c.size());
    CHECK(c.front() == 023, "first code");
    CHECK(c.back() == 0754, "last code");
    for (size_t i = 1; i < c.size(); i++)
        CHECK(c[i] > c[i - 1], "codes increase at index " << i);

    CHECK(dcs_valid(023), "023 is valid");
    CHECK(dcs_valid(0754), "0754 is valid");
    CHECK(!dcs_valid(1), "1 is not valid");
    CHECK(!dcs_valid(0), "0 is not valid");

    CHECK(dcs_word(023, false) == 0x763813u, "word for 023");
    CHECK(dcs_word(023, true) == ((~0x763813u) & 0x7FFFFFu), "inverted word for 023");

    std::vector<float> s = dcs_samples(023, false, 48000.0f, 0.15f);
    CHECK(std::abs((double)s.size() - 23.0 * 48000.0 / 134.4) < 1.5,
          "sample count " << s.size());
}

int main()
{
    test_dcs();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX DCS checks passed" << std::endl;
    return EXIT_SUCCESS;
}
