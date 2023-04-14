/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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

#include <gnuradio/hier_block2.h>
#include <gnuradio/io_signature.h>

#include "fir_decim.h"
#include "fir_decim_coef.h"

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
}

fir_decim_cc::~fir_decim_cc()
{

}
