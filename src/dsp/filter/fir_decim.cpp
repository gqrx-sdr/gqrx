/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2016 Alexandru Csete OZ9AEC.
 * Copyright 2017 Youssef Touil.
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
#include <cstdio>
#include <vector>

#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include "fir_decim.h"
#include "fir_decim_coef.h"

#ifdef USE_NEW_FIR_DECIM
struct decimation_stage
{
    int         decimation;
    int         ratio;
    int         length;
    const float *kernel;
};

static const int decimation_stage_count = 8;
static const decimation_stage decimation_stages[] =
{
    {
        2,
        2,
        d_2_r_2_len,
        d_2_r_2_kernel
    },
    {
        4,
        4,
        d_4_r_4_len,
        d_4_r_4_kernel
    },
    {
        8,
        8,
        d_8_r_8_len,
        d_8_r_8_kernel
    },
    {
        16,
        8,
        d_16_r_8_len,
        d_16_r_8_kernel
    },
    {
        32,
        16,
        d_32_r_16_len,
        d_32_r_16_kernel
    },
    {
        64,
        32,
        d_64_r_32_len,
        d_64_r_32_kernel
    },
    {
        128,
        32,
        d_128_r_32_len,
        d_128_r_32_kernel
    },
    {
        256,
        64,
        d_256_r_64_len,
        d_256_r_64_kernel
    }
};
#endif

fir_decim_cc_sptr make_fir_decim_cc(unsigned int decim)
{
    return gnuradio::get_initial_sptr(new fir_decim_cc(decim));
}

fir_decim_cc::fir_decim_cc(unsigned int decim)
    : gr::hier_block2("fir_decim_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    std::vector<float>  taps;

#ifdef USE_NEW_FIR_DECIM
    int this_stage = 0;
    int index = decimation_stage_count - 1;

    std::cout << "Decimation: " << decim << std::endl;
    while (decim > 1 && index >= 0)
    {
        const decimation_stage  *stage = &decimation_stages[index];

        if (decim % stage->decimation == 0)
        {
            this_stage++;
            taps.assign(stage->kernel, stage->kernel + stage->length);
            if (this_stage == 1)
                fir1 = gr::filter::fir_filter_ccf::make(stage->ratio, taps);
            else if (this_stage == 2)
                fir2 = gr::filter::fir_filter_ccf::make(stage->ratio, taps);
            else if (this_stage == 3)  // NB: currently max 2 stages
                fir3 = gr::filter::fir_filter_ccf::make(stage->ratio, taps);
            else
                std::cout << "  Too many decimation stages: " << this_stage
                          << std::endl;

            std::cout << "  stage: " << this_stage << "  ratio: " << stage->ratio
                      << std::endl;
            decim /= stage->ratio;
        }
        else
        {
            index--;
        }
    }

    if (this_stage == 1)
    {
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, self(), 0);
    }
    else if (this_stage == 2)
    {
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, self(), 0);
    }
    else
    {
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, fir3, 0);
        connect(fir3, 0, self(), 0);
    }

#else
    switch (decim)
    {
    default:
        std::cout << "Invalid decimation: " << decim
                  << " (falling back to 2)." << std::endl;
        // fallthrough
    case 2:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.4500000000    0.5500000000    2
        taps.assign(FIR_2_1_TAPS, FIR_2_1_TAPS + FIR_2_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, self(), 0);
        break;

    case 4:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.2250000000    0.7750000000    2
        // FIR2    0.4500000000    0.5500000000    2
        taps.assign(FIR_4_1_TAPS, FIR_4_1_TAPS + FIR_4_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(2, taps);
        taps.assign(FIR_4_2_TAPS, FIR_4_2_TAPS + FIR_4_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, self(), 0);
        break;

    case 8:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.1125000000    0.3875000000    4
        // FIR2    0.4500000000    0.5500000000    2
        taps.assign(FIR_8_1_TAPS, FIR_8_1_TAPS + FIR_8_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(4, taps);
        taps.assign(FIR_8_2_TAPS, FIR_8_2_TAPS + FIR_8_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, self(), 0);
        break;

    case 16:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.0562500000    0.4437500000    4
        // FIR2    0.2250000000    0.7750000000    2
        // FIR3    0.4500000000    0.5500000000    2
        taps.assign(FIR_16_1_TAPS, FIR_16_1_TAPS + FIR_16_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(4, taps);
        taps.assign(FIR_16_2_TAPS, FIR_16_2_TAPS + FIR_16_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(2, taps);
        taps.assign(FIR_16_3_TAPS, FIR_16_3_TAPS + FIR_16_3_LEN);
        fir3 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, fir3, 0);
        connect(fir3, 0, self(), 0);
        break;

    case 32:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.0281250000    0.2218750000    8
        // FIR2    0.2250000000    0.7750000000    2
        // FIR3    0.4500000000    0.5500000000    2
        taps.assign(FIR_32_1_TAPS, FIR_32_1_TAPS + FIR_32_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(8, taps);
        taps.assign(FIR_32_2_TAPS, FIR_32_2_TAPS + FIR_32_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(2, taps);
        taps.assign(FIR_32_3_TAPS, FIR_32_3_TAPS + FIR_32_3_LEN);
        fir3 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, fir3, 0);
        connect(fir3, 0, self(), 0);
        break;

    case 64:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.0140625000    0.2359375000    8
        // FIR2    0.1125000000    0.3875000000    4
        // FIR3    0.4500000000    0.5500000000    2
        taps.assign(FIR_64_1_TAPS, FIR_64_1_TAPS + FIR_64_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(8, taps);
        taps.assign(FIR_64_2_TAPS, FIR_64_2_TAPS + FIR_64_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(4, taps);
        taps.assign(FIR_64_3_TAPS, FIR_64_3_TAPS + FIR_64_3_LEN);
        fir3 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, fir3, 0);
        connect(fir3, 0, self(), 0);
        break;

    case 128:
        // Stage   Passband        Stopband        Decimation
        // FIR1    0.0070312500    0.1179688000    16
        // FIR2    0.1125000000    0.3875000000    4
        // FIR3    0.4500000000    0.5500000000    2
        taps.assign(FIR_128_1_TAPS, FIR_128_1_TAPS + FIR_128_1_LEN);
        fir1 = gr::filter::fir_filter_ccf::make(16, taps);
        taps.assign(FIR_128_2_TAPS, FIR_128_2_TAPS + FIR_128_2_LEN);
        fir2 = gr::filter::fir_filter_ccf::make(4, taps);
        taps.assign(FIR_128_3_TAPS, FIR_128_3_TAPS + FIR_128_3_LEN);
        fir3 = gr::filter::fir_filter_ccf::make(2, taps);
        connect(self(), 0, fir1, 0);
        connect(fir1, 0, fir2, 0);
        connect(fir2, 0, fir3, 0);
        connect(fir3, 0, self(), 0);
        break;
    }
#endif
}

fir_decim_cc::~fir_decim_cc()
{

}
