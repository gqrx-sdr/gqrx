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

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,5> * out, int noutput_items)
{
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*out)[0];
        i = std::round(in->real()*d_scale);
        q = std::round(in->imag()*d_scale);
        in++;
        p[0] = i & 0xff;
        p[1] = ((i & 0x0300) >> 8) | ((q & 0x3f) << 2);
        p[2] = (q >> 6) & 0x0f;
        i = std::round(in->real()*d_scale);
        q = std::round(in->imag()*d_scale);
        in++;
        p[2] |= (i & 0x0f) << 4;
        p[3] = ((i & 0x3f0) >> 4) | ((q & 0x03) << 6);
        p[4] = q >> 2;
        out++;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,5> *in, gr_complex * out, int noutput_items)
{
    noutput_items&=-2;
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*in)[0];
        i = p[0] | ((p[1] & 0x03) << 8);
        q = (p[1] >> 2) | ((p[2] & 0x0f) << 6);
        *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
        out++;
        i = (p[2] >> 4) | ((p[3] & 0x3f) << 4);
        q = (p[3] >> 6) | (p[4] << 2);
        *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
        out++;
        in++;
        noutput_items-=2;
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,3> * out, int noutput_items)
{
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*out)[0];
        i = std::round(in->real()*d_scale);
        q = std::round(in->imag()*d_scale);
        in++;
        p[0] = i & 0xff;
        p[1] = (i & 0x0f00) >> 8 | (q & 0x0f)<<4;
        p[2] = q >> 4;
        out++;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,3> *in, gr_complex * out, int noutput_items)
{
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*in)[0];
        i = p[0] | (p[1] & 0x0f) << 8;
        q = p[1] >> 4 | p[2] << 4;
        *out=gr_complex(float((i&(1<<11))?i-4096:i)*d_scale_i, float((q&(1<<11))?q-4096:q)*d_scale_i);
        out++;
        in++;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,7> *in, gr_complex * out, int noutput_items)
{
    noutput_items&=-2;
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*in)[0];
        i = p[0] | ((p[1] & 0x3f) << 8);
        q = (p[1] >> 6) | (p[2] << 2) | ((p[3] & 0x0f) << 10);
        *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
        out++;
        i = (p[3] >> 4) | (p[4] << 4) | ((p[5] & 0x03) << 12);
        q = (p[5] >> 2) | (p[6] << 6);
        *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
        out++;
        in++;
        noutput_items-=2;
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,7> * out, int noutput_items)
{
    while(noutput_items)
    {
        int i;
        int q;
        uint8_t * p = (uint8_t *) &(*out)[0];
        i = std::round(in->real()*d_scale);
        q = std::round(in->imag()*d_scale);
        in++;
        p[0] = i & 0xff;
        p[1] = ((i & 0x3f00) >> 8) | ((q & 0x03) << 6);
        p[2] = (q >> 2) & 0xff;
        p[3] = (q >> 10) & 0x0f;
        i = std::round(in->real()*d_scale);
        q = std::round(in->imag()*d_scale);
        in++;
        p[3] |= (i & 0x0f) << 4;
        p[4] = (i >> 4) & 0xff;
        p[5] = ((i >> 12) & 0x03) | ((q & 0x3f) << 2);
        p[6] = q >> 6;
        out++;
        noutput_items--;
    }
}

constexpr std::array<any_to_any_base::format_descriptor, FILE_FORMAT_COUNT> any_to_any_base::fmt;
