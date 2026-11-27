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

/* Tests for the TX device string with USRP, HackRF and PlutoSDR devices. */
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

static void test_devices()
{
    CHECK(tx_device_string("uhd", "rtl=0") == "uhd", "usrp");
    CHECK(tx_device_string("uhd,type=b200", "") == "uhd,type=b200", "usrp b200");
    CHECK(tx_device_string("hackrf=0", "rtl=0") == "hackrf=0", "hackrf");
    CHECK(tx_device_string("plutosdr,uri=usb:1.2.3", "") == "plutosdr,uri=usb:1.2.3",
          "plutosdr");

    /* fallback to the input device */
    CHECK(tx_device_string("", "hackrf=0") == "hackrf=0", "hackrf input");
    CHECK(tx_device_string("", "uhd,type=b200") == "uhd,type=b200", "usrp input");
    CHECK(tx_device_string("", "plutosdr,uri=usb:1.2.3") == "plutosdr,uri=usb:1.2.3",
          "plutosdr input");

    /* a file cannot transmit */
    CHECK(tx_device_string("", "file=/tmp/x.raw").empty(), "file input");
    CHECK(tx_device_string("plutosdr", "file=/tmp/x.raw") == "plutosdr",
          "tx device with file input");
}

int main()
{
    test_devices();

    if (failures)
    {
        std::cerr << failures << " check(s) failed" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All TX device checks passed" << std::endl;
    return EXIT_SUCCESS;
}
