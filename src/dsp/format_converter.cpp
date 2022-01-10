/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2021 vladisslav2011@gmail.com.
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

#include "format_converter.h"

void any_to_any_impl::convert(const gr_complex *in, gr_complex * out, int noutput_items)
{
    memcpy(out, in, noutput_items * sizeof(gr_complex));
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<int32_t> * out, int noutput_items)
{
    volk_32f_s32f_convert_32i((int32_t *)out, (const float *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const std::complex<int32_t> *in, gr_complex * out, int noutput_items)
{
    volk_32i_s32f_convert_32f((float *)out, (const int32_t *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<uint32_t> * out, int noutput_items)
{
    volk_32f_s32f_convert_32i((int32_t *)out, (const float *)in, d_scale, noutput_items * 2);
    uint32_t *out_u = (uint32_t *) out;
    for(int k = 0;k < noutput_items;k++)
    {
        *(out_u++) += uint32_t(INT32_MAX) + 1;
        *(out_u++) += uint32_t(INT32_MAX) + 1;
    }
}

void any_to_any_impl::convert(const std::complex<uint32_t> *in, gr_complex * out, int noutput_items)
{

    for(int k = 0;k < noutput_items;k++,in++,out++)
    {
        *out = gr_complex((float(in->real())+float(INT32_MIN)) * d_scale_i, (float(in->imag())+float(INT32_MIN)) * d_scale_i);
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<int16_t> * out, int noutput_items)
{
    volk_32f_s32f_convert_16i((int16_t *)out, (const float *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const std::complex<int16_t> *in, gr_complex * out, int noutput_items)
{
    volk_16i_s32f_convert_32f((float *)out, (const int16_t *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<uint16_t> * out, int noutput_items)
{
    uint16_t *out_u = (uint16_t *) out;
    volk_32f_s32f_convert_16i((int16_t *)out, (const float *)in, d_scale, noutput_items * 2);
    for(int k = 0;k < noutput_items;k++)
    {
        *(out_u++) += INT16_MAX + 1;
        *(out_u++) += INT16_MAX + 1;
    }
}

void any_to_any_impl::convert(const std::complex<uint16_t> *in, gr_complex * out, int noutput_items)
{
    for(int k = 0;k < noutput_items;k++,in++,out++)
    {
        *out = gr_complex((float(in->real())+float(INT16_MIN)) * d_scale_i, (float(in->imag())+float(INT16_MIN)) * d_scale_i);
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<int8_t> * out, int noutput_items)
{
    volk_32f_s32f_convert_8i((int8_t *)out, (const float *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const std::complex<int8_t> *in, gr_complex * out, int noutput_items)
{
    volk_8i_s32f_convert_32f((float *)out, (const int8_t *)in, d_scale, noutput_items * 2);
}

void any_to_any_impl::convert(const gr_complex *in, std::complex<uint8_t> * out, int noutput_items)
{
    uint8_t *out_u = (uint8_t *) out;
    volk_32f_s32f_convert_8i((int8_t *)out, (const float *)in, d_scale, noutput_items * 2);
    for(int k = 0;k < noutput_items;k++)
    {
        *(out_u++) += INT8_MAX + 1;
        *(out_u++) += INT8_MAX + 1;
    }
}

void any_to_any_impl::convert(const std::complex<uint8_t> *in, gr_complex * out, int noutput_items)
{
    for(int k = 0;k < noutput_items;k++,in++,out++)
    {
        *out = gr_complex((float(in->real())+float(INT8_MIN)) * d_scale_i, (float(in->imag())+float(INT8_MIN)) * d_scale_i);
    }
}

constexpr std::array<any_to_any_base::format_descriptor, FILE_FORMAT_COUNT> any_to_any_base::fmt;
