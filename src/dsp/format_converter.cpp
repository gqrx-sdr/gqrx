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

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*out)[0];
    noutput_items*=8;
    while(noutput_items)
    {
        int i;
        int q;
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
        p+=5;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i;
        int q;
        i = p[0] | ((p[1] & 0x03) << 8);
        q = (p[1] >> 2) | ((p[2] & 0x0f) << 6);
        *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
        out++;
        i = (p[2] >> 4) | ((p[3] & 0x3f) << 4);
        q = (p[3] >> 6) | (p[4] << 2);
        *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
        out++;
        p+=5;
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

////////////////////////////////
// 32 bit accelerated converters
////////////////////////////////

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
        uint32_t * r = (uint32_t *) buf;
        *p =  (*r & 0x000003ff)|
             ((*r & 0x03ff0000) >> 6);
        r++;
        *p|= ((*r & 0x000003ff) << 20)|//0>>20
             ((*r & 0x00030000) << 14);//16>>30
        p++;
        *p = ((*r & 0x03fc0000) >> 18);//16>>-2
        r++;
        *p|= ((*r & 0x000003ff) << 8)|//0>>8
             ((*r & 0x03ff0000) << 2);//16>>18
        r++;
        *p|= ((*r & 0x0000000f) << 28);//0>>28
        p++;
        *p = ((*r & 0x000003f0) >> 4)|//0>>-4
             ((*r & 0x03ff0000) >> 10);//16>>6
        r++;
        *p|= ((*r & 0x000003ff) << 16)|//0>>16
             ((*r & 0x003f0000) << 10);//16>>26
        p++;
        *p = ((*r & 0x03c00000) >> 22);//16>>-6
        r++;
        *p|= ((*r & 0x000003ff) << 4)|//0>>4
             ((*r & 0x03ff0000) >> 2);//16>>14
        r++;
        *p|= ((*r & 0x000000ff) << 24);//0>>24
        p++;
        *p = ((*r & 0x00000300) >> 8)|//0>>-8
             ((*r & 0x03ff0000) >> 14);//16>>2
        r++;
        *p|= ((*r & 0x000003ff) << 12)|//0>>12
             ((*r & 0x03ff0000) << 6 );//16>>22
        p++;
        in+=8;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o = (*i & 0x000003ff) << 6;
        *o|= (*i & 0x000ffc00) << 12;
        o++;
        *o = (*i & 0x3ff00000) >> 14;
        *o|= (*i & 0xc0000000) >> 8;
        i++;
        *o|= (*i & 0x000000ff) << 24;
        o++;
        *o = (*i & 0x0003ff00) >> 2;
        *o|= (*i & 0x0ffc0000) << 4;
        o++;
        *o = (*i & 0xf0000000) >> 22;
        i++;
        *o|= (*i & 0x0000003f) << 10;
        *o|= (*i & 0x0000ffc0) << 16;
        o++;
        *o = (*i & 0x03ff0000) >> 10;
        *o|= (*i & 0xfc000000) >> 4;
        i++;
        *o|= (*i & 0x0000000f) << 28;
        o++;
        *o = (*i & 0x00003ff0) << 2;
        *o|= (*i & 0x00ffc000) << 8;
        o++;
        *o = (*i & 0xff000000) >> 18;
        i++;
        *o|= (*i & 0x00000003) << 14;
        *o|= (*i & 0x00000ffc) << 20;
        o++;
        *o = (*i & 0x003ff000) >> 6;
        *o|= (*i & 0xffc00000);
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
        out+=8;
        noutput_items-=8;
    }
}

////////////////////////////////
// 64 bit accelerated converters
////////////////////////////////

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint64_t * r = (uint64_t *) buf;
        *p =  (*r & 0x00000000000003ff)|
             ((*r & 0x0000000003ff0000) >> 6 )|
             ((*r & 0x000003ff00000000) >> 12)|
             ((*r & 0x03ff000000000000) >> 18);
        r++;
        *p|= ((*r & 0x00000000000003ff) << 40)|//0>>40
             ((*r & 0x0000000003ff0000) << 34)|//16>>50
             ((*r & 0x0000000f00000000) << 28);//32>>60
        p++;
        *p = ((*r & 0x000003f000000000) >> 36)|//32>>-4
             ((*r & 0x03ff000000000000) >> 42);//48>>6
        r++;
        *p|= ((*r & 0x00000000000003ff) << 16)|//0>>16
             ((*r & 0x0000000003ff0000) << 10)|//16>>26
             ((*r & 0x000003ff00000000) << 4 )|//32>>36
             ((*r & 0x03ff000000000000) >> 2 );//48>>46
        r++;
        *p|= ((*r & 0x00000000000000ff) << 56);//0>>56
        p++;
        *p = ((*r & 0x0000000000000300) >> 8 )|//0>>-8
             ((*r & 0x0000000003ff0000) >> 14)|//16>>2
             ((*r & 0x000003ff00000000) >> 20)|//32>>12
             ((*r & 0x03ff000000000000) >> 26);//48>>22
        r++;
        *p|= ((*r & 0x00000000000003ff) << 32)|//0>>32
             ((*r & 0x0000000003ff0000) << 26)|//16>>42
             ((*r & 0x000003ff00000000) << 20)|//32>>52
             ((*r & 0x0003000000000000) << 14);//48>>62
        p++;
        *p = ((*r & 0x03fc000000000000) >> 50);//48>>-2
        r++;
        *p|= ((*r & 0x00000000000003ff) << 8 )|//0>>8
             ((*r & 0x0000000003ff0000) << 2 )|//16>>18
             ((*r & 0x000003ff00000000) >> 4 )|//32>>28
             ((*r & 0x03ff000000000000) >> 10);//48>>38
        r++;
        *p|= ((*r & 0x00000000000003ff) << 48)|//0>>48
             ((*r & 0x00000000003f0000) << 42);//16>>58
        p++;
        *p = ((*r & 0x0000000003c00000) >> 22)|//16>>-6
             ((*r & 0x000003ff00000000) >> 28)|//32>>4
             ((*r & 0x03ff000000000000) >> 34);//48>>14
        r++;
        *p|= ((*r & 0x00000000000003ff) << 24)|//0>>24
             ((*r & 0x0000000003ff0000) << 18)|//16>>34
             ((*r & 0x000003ff00000000) << 12)|//32>>44
             ((*r & 0x03ff000000000000) << 6 );//48>>54
        p++;
        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o = (*i & 0x00000000000003ff) << 6;
        *o|= (*i & 0x00000000000ffc00) << 12;
        *o|= (*i & 0x000000003ff00000) << 18;
        *o|= (*i & 0x000000ffc0000000) << 24;
        o++;
        *o = (*i & 0x0003ff0000000000) >> 34;
        *o|= (*i & 0x0ffc000000000000) >> 28;
        *o|= (*i & 0xf000000000000000) >> 22;
        i++;
        *o|= (*i & 0x000000000000003f) << 42;
        *o|= (*i & 0x000000000000ffc0) << 48;
        o++;
        *o = (*i & 0x0000000003ff0000) >> 10;
        *o|= (*i & 0x0000000ffc000000) >> 4;
        *o|= (*i & 0x00003ff000000000) << 2;
        *o|= (*i & 0x00ffc00000000000) << 8;
        o++;
        *o = (*i & 0xff00000000000000) >> 50;
        i++;
        *o|= (*i & 0x0000000000000003) << 14;
        *o|= (*i & 0x0000000000000ffc) << 20;
        *o|= (*i & 0x00000000003ff000) << 26;
        *o|= (*i & 0x00000000ffc00000) << 32;
        o++;
        *o = (*i & 0x000003ff00000000) >> 26;
        *o|= (*i & 0x000ffc0000000000) >> 20;
        *o|= (*i & 0x3ff0000000000000) >> 14;
        *o|= (*i & 0xc000000000000000) >> 8;
        i++;
        *o|= (*i & 0x00000000000000ff) << 56;
        o++;
        *o = (*i & 0x000000000003ff00) >> 2;
        *o|= (*i & 0x000000000ffc0000) << 4;
        *o|= (*i & 0x0000003ff0000000) << 10;
        *o|= (*i & 0x0000ffc000000000) << 16;
        o++;
        *o = (*i & 0x03ff000000000000) >> 42;
        *o|= (*i & 0xfc00000000000000) >> 36;
        i++;
        *o|= (*i & 0x000000000000000f) << 28;
        *o|= (*i & 0x0000000000003ff0) << 34;
        *o|= (*i & 0x0000000000ffc000) << 40;
        o++;
        *o = (*i & 0x00000003ff000000) >> 18;
        *o|= (*i & 0x00000ffc00000000) >> 12;
        *o|= (*i & 0x003ff00000000000) >> 6;
        *o|= (*i & 0xffc0000000000000);

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        in++;
        noutput_items-=16;
    }
}

#ifdef __BMI2__
/////////////////////////////////////
// 64 bit bmi2 accelerated converters
/////////////////////////////////////

void any_to_any_impl_bmi64::convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint64_t * r = (uint64_t *) buf;
        uint64_t t;
        uint64_t b;
        b=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        t=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        b|=t<<40;

        *p=b;
        p++;

        b=_pext_u64(*r,0x03ff03ff03ff03ff)<<16;//80-64=16
        r++;
        b|=t>>24;//64-40=24
        t=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        b|=t<<56;//80-24=56

        *p=b;
        p++;

        b=t>>8;//64-56=8
        t=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        b|=t<<32;//40-8=32

        *p=b;
        p++;

        b=_pext_u64(*r,0x03ff03ff03ff03ff)<<8;//40-32=8
        r++;
        b|=t>>32;//40-8=32
        t=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        b|=t<<48;//80-32=48

        *p=b;
        p++;

        b=t>>16;//64-48=16
        t=_pext_u64(*r,0x03ff03ff03ff03ff);
        r++;
        b|=t<<24;//40-16=24

        *p=b;
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_bmi64::convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    noutput_items&=-2;
    uint64_t t;
    uint64_t * i = (uint64_t *) &(*in)[0];
    while(noutput_items)
    {
        uint64_t * o = (uint64_t *) buf;
        *o = _pdep_u64(*i, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 40;
        i++;
        t |= *i << 24;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 16;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 56;
        i++;
        t |= *i << 8;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 32;
        i++;
        t |= *i << 32;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 8;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 48;
        i++;
        t |= *i << 16;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        o++;
        t = *i >> 24;
        *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
        i++;
        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        noutput_items-=16;
    }
}
#endif

constexpr std::array<any_to_any_base::format_descriptor, FILE_FORMAT_COUNT> any_to_any_base::fmt;
