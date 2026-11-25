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

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,24> * out, int noutput_items)
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
        p[1] = (i & 0x0f00) >> 8 | (q & 0x0f)<<4;
        p[2] = q >> 4;
        p+=3;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,24> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i;
        int q;
        i = p[0] | (p[1] & 0x0f) << 8;
        q = p[1] >> 4 | p[2] << 4;
        *out=gr_complex(float((i&(1<<11))?i-4096:i)*d_scale_i, float((q&(1<<11))?q-4096:q)*d_scale_i);
        out++;
        p+=3;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int8_t,56> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i;
        int q;
        i = p[0] | ((p[1] & 0x3f) << 8);
        q = (p[1] >> 6) | (p[2] << 2) | ((p[3] & 0x0f) << 10);
        *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
        out++;
        i = (p[3] >> 4) | (p[4] << 4) | ((p[5] & 0x03) << 12);
        q = (p[5] >> 2) | (p[6] << 6);
        *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
        out++;
        p+=7;
        noutput_items-=2;
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int8_t,56> * out, int noutput_items)
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
        p+=7;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const gr_complex *in, int8_t * out, int noutput_items)
{
    for(int k=0;k<noutput_items;k++,out++,in++)
        *out=std::round(in->real()*d_scale);
}

void any_to_any_impl::convert(const int8_t *in, gr_complex * out, int noutput_items)
{
    for(int k=0;k<noutput_items;k++,out++,in++)
        *out=gr_complex(*in * d_scale_i);
}

void any_to_any_impl::convert(const gr_complex *in, int16_t * out, int noutput_items)
{
    for(int k=0;k<noutput_items;k++,out++,in++)
        *out=std::round(in->real()*d_scale);
}

void any_to_any_impl::convert(const int16_t *in, gr_complex * out, int noutput_items)
{
    for(int k=0;k<noutput_items;k++,out++,in++)
        *out=gr_complex(*in * d_scale_i);
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int16_t,20> * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*out)[0];
    noutput_items*=8;
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = std::round(in->real()*d_scale);
        in++;
        i1 = std::round(in->real()*d_scale);
        in++;
        p[0] = i0 & 0xff;
        p[1] = ((i0 & 0x0300) >> 8) | ((i1 & 0x3f) << 2);
        p[2] = (i1 >> 6) & 0x0f;
        i0 = std::round(in->real()*d_scale);
        in++;
        i1 = std::round(in->real()*d_scale);
        in++;
        p[2] |= (i0 & 0x0f) << 4;
        p[3] = ((i0 & 0x3f0) >> 4) | ((i1 & 0x03) << 6);
        p[4] = i1 >> 2;
        p+=5;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int16_t,20> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = p[0] | ((p[1] & 0x03) << 8);
        i1 = (p[1] >> 2) | ((p[2] & 0x0f) << 6);
        *out=gr_complex(float((i0&(1<<9))?i0-1024:i0)*d_scale_i);
        out++;
        *out=gr_complex(float((i1&(1<<9))?i1-1024:i1)*d_scale_i);
        out++;
        i0 = (p[2] >> 4) | ((p[3] & 0x3f) << 4);
        i1 = (p[3] >> 6) | (p[4] << 2);
        *out=gr_complex(float((i0&(1<<9))?i0-1024:i0)*d_scale_i);
        out++;
        *out=gr_complex(float((i1&(1<<9))?i1-1024:i1)*d_scale_i);
        out++;
        p+=5;
        noutput_items-=2;
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int16_t,12> * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*out)[0];
    noutput_items*=8;
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = std::round(in->real()*d_scale);
        in++;
        i1 = std::round(in->real()*d_scale);
        in++;
        p[0] = i0 & 0xff;
        p[1] = (i0 & 0x0f00) >> 8 | (i1 & 0x0f)<<4;
        p[2] = i1 >> 4;
        p+=3;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int16_t,12> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = p[0] | (p[1] & 0x0f) << 8;
        i1 = p[1] >> 4 | p[2] << 4;
        *out=gr_complex(float((i0&(1<<11))?i0-4096:i0)*d_scale_i);
        out++;
        *out=gr_complex(float((i1&(1<<11))?i1-4096:i1)*d_scale_i);
        out++;
        p+=3;
        noutput_items--;
    }
}

void any_to_any_impl::convert(const std::array<int16_t,28> *in, gr_complex * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*in)[0];
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = p[0] | ((p[1] & 0x3f) << 8);
        i1 = (p[1] >> 6) | (p[2] << 2) | ((p[3] & 0x0f) << 10);
        *out=gr_complex(float((i0&(1<<13))?i0-16384:i0)*d_scale_i);
        out++;
        *out=gr_complex(float((i1&(1<<13))?i1-16384:i1)*d_scale_i);
        out++;
        i0 = (p[3] >> 4) | (p[4] << 4) | ((p[5] & 0x03) << 12);
        i1 = (p[5] >> 2) | (p[6] << 6);
        *out=gr_complex(float((i0&(1<<13))?i0-16384:i0)*d_scale_i);
        out++;
        *out=gr_complex(float((i1&(1<<13))?i1-16384:i1)*d_scale_i);
        out++;
        p+=7;
        noutput_items-=4;
    }
}

void any_to_any_impl::convert(const gr_complex *in, std::array<int16_t,28> * out, int noutput_items)
{
    uint8_t * p = (uint8_t *) &(*out)[0];
    noutput_items*=8;
    while(noutput_items)
    {
        int i0;
        int i1;
        i0 = std::round(in->real()*d_scale);
        in++;
        i1 = std::round(in->real()*d_scale);
        in++;
        p[0] = i0 & 0xff;
        p[1] = ((i0 & 0x3f00) >> 8) | ((i1 & 0x03) << 6);
        p[2] = (i1 >> 2) & 0xff;
        p[3] = (i1 >> 10) & 0x0f;
        i0 = std::round(in->real()*d_scale);
        in++;
        i1 = std::round(in->real()*d_scale);
        in++;
        p[3] |= (i0 & 0x0f) << 4;
        p[4] = (i0 >> 4) & 0xff;
        p[5] = ((i0 >> 12) & 0x03) | ((i1 & 0x3f) << 2);
        p[6] = i1 >> 6;
        p+=7;
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

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int8_t,24> * out, int noutput_items)
{
    uint8_t buf[4*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 8);
        uint32_t * r = (uint32_t *) buf;
        *p  = (*r & 0x00000fff);        //0>>0
        *p |= ((*r & 0x0fff0000) >>  4);//16>>12
        r++;
        *p |= ((*r & 0x00000fff) << 24);//0>>24
        p++;
        *p  = ((*r & 0x00000fff) >>  8);//0>>-8
        *p |= ((*r & 0x0fff0000) >> 12);//16>>4
        r++;
        *p |= ((*r & 0x00000fff) << 16);//0>>16
        *p |= ((*r & 0x0fff0000) << 12);//16>>28
        p++;
        *p  = ((*r & 0x0fff0000) >> 20);//16>>-4
        r++;
        *p |= ((*r & 0x00000fff) <<  8);//0>>8
        *p |= ((*r & 0x0fff0000) <<  4);//16>>20
        p++;
        in+=4;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int8_t,24> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[4*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o  = ((*i & 0x00000fff) <<  4);//0>>4
        *o |= ((*i & 0x00fff000) <<  8);//12>>20
        o++;
        *o  = ((*i & 0xff000000) >> 20);//24>>4
        i++;
        *o |= ((*i & 0x0000000f) << 12);//4>>4
        *o |= ((*i & 0x0000fff0) << 16);//4>>20
        o++;
        *o  = ((*i & 0x0fff0000) >> 12);//16>>4
        *o |= ((*i & 0xf0000000) >>  8);//28>>20
        i++;
        *o |= ((*i & 0x000000ff) << 24);//8>>20
        o++;
        *o  = ((*i & 0x000fff00) >>  4);//8>>4
        *o |= (*i & 0xfff00000);        //20>>20
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,8);
        out+=4;
        noutput_items-=4;
    }
}

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int8_t,56> * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
        uint32_t * r = (uint32_t *) buf;
        *p  = (*r & 0x00003fff);        //0>>0
        *p |= ((*r & 0x3fff0000) >>  2);//16>>14
        r++;
        *p |= ((*r & 0x00003fff) << 28);//0>>28
        p++;
        *p  = ((*r & 0x00003fff) >>  4);//0>>-4
        *p |= ((*r & 0x3fff0000) >>  6);//16>>10
        r++;
        *p |= ((*r & 0x00003fff) << 24);//0>>24
        p++;
        *p  = ((*r & 0x00003fff) >>  8);//0>>-8
        *p |= ((*r & 0x3fff0000) >> 10);//16>>6
        r++;
        *p |= ((*r & 0x00003fff) << 20);//0>>20
        p++;
        *p  = ((*r & 0x00003fff) >> 12);//0>>-12
        *p |= ((*r & 0x3fff0000) >> 14);//16>>2
        r++;
        *p |= ((*r & 0x00003fff) << 16);//0>>16
        *p |= ((*r & 0x3fff0000) << 14);//16>>30
        p++;
        *p  = ((*r & 0x3fff0000) >> 18);//16>>-2
        r++;
        *p |= ((*r & 0x00003fff) << 12);//0>>12
        *p |= ((*r & 0x3fff0000) << 10);//16>>26
        p++;
        *p  = ((*r & 0x3fff0000) >> 22);//16>>-6
        r++;
        *p |= ((*r & 0x00003fff) <<  8);//0>>8
        *p |= ((*r & 0x3fff0000) <<  6);//16>>22
        p++;
        *p  = ((*r & 0x3fff0000) >> 26);//16>>-10
        r++;
        *p |= ((*r & 0x00003fff) <<  4);//0>>4
        *p |= ((*r & 0x3fff0000) <<  2);//16>>18
        p++;

        in+=8;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int8_t,56> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o  = ((*i & 0x00003fff) <<  2);//0>>2
        *o |= ((*i & 0x0fffc000) <<  4);//14>>18
        o++;
        *o  = ((*i & 0xf0000000) >> 26);//28>>2
        i++;
        *o |= ((*i & 0x000003ff) <<  6);//10>>2
        *o |= ((*i & 0x00fffc00) <<  8);//10>>18
        o++;
        *o  = ((*i & 0xff000000) >> 22);//24>>2
        i++;
        *o |= ((*i & 0x0000003f) << 10);//6>>2
        *o |= ((*i & 0x000fffc0) << 12);//6>>18
        o++;
        *o  = ((*i & 0xfff00000) >> 18);//20>>2
        i++;
        *o |= ((*i & 0x00000003) << 14);//2>>2
        *o |= ((*i & 0x0000fffc) << 16);//2>>18
        o++;
        *o  = ((*i & 0x3fff0000) >> 14);//16>>2
        *o |= ((*i & 0xc0000000) >> 12);//30>>18
        i++;
        *o |= ((*i & 0x00000fff) << 20);//12>>18
        o++;
        *o  = ((*i & 0x03fff000) >> 10);//12>>2
        *o |= ((*i & 0xfc000000) >>  8);//26>>18
        i++;
        *o |= ((*i & 0x000000ff) << 24);//8>>18
        o++;
        *o  = ((*i & 0x003fff00) >>  6);//8>>2
        *o |= ((*i & 0xffc00000) >>  4);//22>>18
        i++;
        *o |= ((*i & 0x0000000f) << 28);//4>>18
        o++;
        *o  = ((*i & 0x0003fff0) >>  2);//4>>2
        *o |= (*i & 0xfffc0000);        //18>>18
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
        out+=8;
        noutput_items-=8;
    }
}

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int16_t,20> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint32_t * r = (uint32_t *) buf;
        *p  = (*r & 0x000003ff);        //0>>0
        r++;
        *p |= ((*r & 0x000003ff) << 10);//0>>10
        r++;
        *p |= ((*r & 0x000003ff) << 20);//0>>20
        r++;
        *p |= ((*r & 0x000003ff) << 30);//0>>30
        p++;
        *p  = ((*r & 0x000003ff) >>  2);//0>>-2
        r++;
        *p |= ((*r & 0x000003ff) <<  8);//0>>8
        r++;
        *p |= ((*r & 0x000003ff) << 18);//0>>18
        r++;
        *p |= ((*r & 0x000003ff) << 28);//0>>28
        p++;
        *p  = ((*r & 0x000003ff) >>  4);//0>>-4
        r++;
        *p |= ((*r & 0x000003ff) <<  6);//0>>6
        r++;
        *p |= ((*r & 0x000003ff) << 16);//0>>16
        r++;
        *p |= ((*r & 0x000003ff) << 26);//0>>26
        p++;
        *p  = ((*r & 0x000003ff) >>  6);//0>>-6
        r++;
        *p |= ((*r & 0x000003ff) <<  4);//0>>4
        r++;
        *p |= ((*r & 0x000003ff) << 14);//0>>14
        r++;
        *p |= ((*r & 0x000003ff) << 24);//0>>24
        p++;
        *p  = ((*r & 0x000003ff) >>  8);//0>>-8
        r++;
        *p |= ((*r & 0x000003ff) <<  2);//0>>2
        r++;
        *p |= ((*r & 0x000003ff) << 12);//0>>12
        r++;
        *p |= ((*r & 0x000003ff) << 22);//0>>22
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int16_t,20> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o  = ((*i & 0x000003ff) <<  6);//0>>6
        o++;
        *o  = ((*i & 0x000ffc00) >>  4);//10>>6
        o++;
        *o  = ((*i & 0x3ff00000) >> 14);//20>>6
        o++;
        *o  = ((*i & 0xc0000000) >> 24);//30>>6
        i++;
        *o |= ((*i & 0x000000ff) <<  8);//8>>6
        o++;
        *o  = ((*i & 0x0003ff00) >>  2);//8>>6
        o++;
        *o  = ((*i & 0x0ffc0000) >> 12);//18>>6
        o++;
        *o  = ((*i & 0xf0000000) >> 22);//28>>6
        i++;
        *o |= ((*i & 0x0000003f) << 10);//6>>6
        o++;
        *o  = (*i & 0x0000ffc0);        //6>>6
        o++;
        *o  = ((*i & 0x03ff0000) >> 10);//16>>6
        o++;
        *o  = ((*i & 0xfc000000) >> 20);//26>>6
        i++;
        *o |= ((*i & 0x0000000f) << 12);//4>>6
        o++;
        *o  = ((*i & 0x00003ff0) <<  2);//4>>6
        o++;
        *o  = ((*i & 0x00ffc000) >>  8);//14>>6
        o++;
        *o  = ((*i & 0xff000000) >> 18);//24>>6
        i++;
        *o |= ((*i & 0x00000003) << 14);//2>>6
        o++;
        *o  = ((*i & 0x00000ffc) <<  4);//2>>6
        o++;
        *o  = ((*i & 0x003ff000) >>  6);//12>>6
        o++;
        *o  = ((*i & 0xffc00000) >> 16);//22>>6
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        noutput_items-=16;
    }
}

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int16_t,12> * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
        uint32_t * r = (uint32_t *) buf;
        *p  = (*r & 0x00000fff);        //0>>0
        r++;
        *p |= ((*r & 0x00000fff) << 12);//0>>12
        r++;
        *p |= ((*r & 0x00000fff) << 24);//0>>24
        p++;
        *p  = ((*r & 0x00000fff) >>  8);//0>>-8
        r++;
        *p |= ((*r & 0x00000fff) <<  4);//0>>4
        r++;
        *p |= ((*r & 0x00000fff) << 16);//0>>16
        r++;
        *p |= ((*r & 0x00000fff) << 28);//0>>28
        p++;
        *p  = ((*r & 0x00000fff) >>  4);//0>>-4
        r++;
        *p |= ((*r & 0x00000fff) <<  8);//0>>8
        r++;
        *p |= ((*r & 0x00000fff) << 20);//0>>20
        p++;

        in+=4;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int16_t,12> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o  = ((*i & 0x00000fff) <<  4);//0>>4
        o++;
        *o  = ((*i & 0x00fff000) >>  8);//12>>4
        o++;
        *o  = ((*i & 0xff000000) >> 20);//24>>4
        i++;
        *o |= ((*i & 0x0000000f) << 12);//4>>4
        o++;
        *o  = (*i & 0x0000fff0);        //4>>4
        o++;
        *o  = ((*i & 0x0fff0000) >> 12);//16>>4
        o++;
        *o  = ((*i & 0xf0000000) >> 24);//28>>4
        i++;
        *o |= ((*i & 0x000000ff) <<  8);//8>>4
        o++;
        *o  = ((*i & 0x000fff00) >>  4);//8>>4
        o++;
        *o  = ((*i & 0xfff00000) >> 16);//20>>4
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
        out+=8;
        noutput_items-=8;
    }
}

void any_to_any_impl_32::convert(const gr_complex *in, std::array<int16_t,28> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint32_t * p = (uint32_t *) &(*out)[0];
    noutput_items *= 2;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint32_t * r = (uint32_t *) buf;
        *p  = (*r & 0x00003fff);        //0>>0
        r++;
        *p |= ((*r & 0x00003fff) << 14);//0>>14
        r++;
        *p |= ((*r & 0x00003fff) << 28);//0>>28
        p++;
        *p  = ((*r & 0x00003fff) >>  4);//0>>-4
        r++;
        *p |= ((*r & 0x00003fff) << 10);//0>>10
        r++;
        *p |= ((*r & 0x00003fff) << 24);//0>>24
        p++;
        *p  = ((*r & 0x00003fff) >>  8);//0>>-8
        r++;
        *p |= ((*r & 0x00003fff) <<  6);//0>>6
        r++;
        *p |= ((*r & 0x00003fff) << 20);//0>>20
        p++;
        *p  = ((*r & 0x00003fff) >> 12);//0>>-12
        r++;
        *p |= ((*r & 0x00003fff) <<  2);//0>>2
        r++;
        *p |= ((*r & 0x00003fff) << 16);//0>>16
        r++;
        *p |= ((*r & 0x00003fff) << 30);//0>>30
        p++;
        *p  = ((*r & 0x00003fff) >>  2);//0>>-2
        r++;
        *p |= ((*r & 0x00003fff) << 12);//0>>12
        r++;
        *p |= ((*r & 0x00003fff) << 26);//0>>26
        p++;
        *p  = ((*r & 0x00003fff) >>  6);//0>>-6
        r++;
        *p |= ((*r & 0x00003fff) <<  8);//0>>8
        r++;
        *p |= ((*r & 0x00003fff) << 22);//0>>22
        p++;
        *p  = ((*r & 0x00003fff) >> 10);//0>>-10
        r++;
        *p |= ((*r & 0x00003fff) <<  4);//0>>4
        r++;
        *p |= ((*r & 0x00003fff) << 18);//0>>18
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_32::convert(const std::array<int16_t,28> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint32_t * i = (uint32_t *) &(*in)[0];
    while(noutput_items)
    {
        uint32_t * o = (uint32_t *) buf;
        *o  = ((*i & 0x00003fff) <<  2);//0>>2
        o++;
        *o  = ((*i & 0x0fffc000) >> 12);//14>>2
        o++;
        *o  = ((*i & 0xf0000000) >> 26);//28>>2
        i++;
        *o |= ((*i & 0x000003ff) <<  6);//10>>2
        o++;
        *o  = ((*i & 0x00fffc00) >>  8);//10>>2
        o++;
        *o  = ((*i & 0xff000000) >> 22);//24>>2
        i++;
        *o |= ((*i & 0x0000003f) << 10);//6>>2
        o++;
        *o  = ((*i & 0x000fffc0) >>  4);//6>>2
        o++;
        *o  = ((*i & 0xfff00000) >> 18);//20>>2
        i++;
        *o |= ((*i & 0x00000003) << 14);//2>>2
        o++;
        *o  = (*i & 0x0000fffc);        //2>>2
        o++;
        *o  = ((*i & 0x3fff0000) >> 14);//16>>2
        o++;
        *o  = ((*i & 0xc0000000) >> 28);//30>>2
        i++;
        *o |= ((*i & 0x00000fff) <<  4);//12>>2
        o++;
        *o  = ((*i & 0x03fff000) >> 10);//12>>2
        o++;
        *o  = ((*i & 0xfc000000) >> 24);//26>>2
        i++;
        *o |= ((*i & 0x000000ff) <<  8);//8>>2
        o++;
        *o  = ((*i & 0x003fff00) >>  6);//8>>2
        o++;
        *o  = ((*i & 0xffc00000) >> 20);//22>>2
        i++;
        *o |= ((*i & 0x0000000f) << 12);//4>>2
        o++;
        *o  = ((*i & 0x0003fff0) >>  2);//4>>2
        o++;
        *o  = ((*i & 0xfffc0000) >> 16);//18>>2
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        noutput_items-=16;
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

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int8_t,24> * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
        uint64_t * r = (uint64_t *) buf;
        *p  = (*r & 0x0000000000000fff);        //0>>0
        *p |= ((*r & 0x000000000fff0000) >>  4);//16>>12
        *p |= ((*r & 0x00000fff00000000) >>  8);//32>>24
        *p |= ((*r & 0x0fff000000000000) >> 12);//48>>36
        r++;
        *p |= ((*r & 0x0000000000000fff) << 48);//0>>48
        *p |= ((*r & 0x000000000fff0000) << 44);//16>>60
        p++;
        *p  = ((*r & 0x000000000fff0000) >> 20);//16>>-4
        *p |= ((*r & 0x00000fff00000000) >> 24);//32>>8
        *p |= ((*r & 0x0fff000000000000) >> 28);//48>>20
        r++;
        *p |= ((*r & 0x0000000000000fff) << 32);//0>>32
        *p |= ((*r & 0x000000000fff0000) << 28);//16>>44
        *p |= ((*r & 0x00000fff00000000) << 24);//32>>56
        p++;
        *p  = ((*r & 0x00000fff00000000) >> 40);//32>>-8
        *p |= ((*r & 0x0fff000000000000) >> 44);//48>>4
        r++;
        *p |= ((*r & 0x0000000000000fff) << 16);//0>>16
        *p |= ((*r & 0x000000000fff0000) << 12);//16>>28
        *p |= ((*r & 0x00000fff00000000) <<  8);//32>>40
        *p |= ((*r & 0x0fff000000000000) <<  4);//48>>52
        p++;
        in+=8;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int8_t,24> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o  = ((*i & 0x0000000000000fff) <<  4);//0>>4
        *o |= ((*i & 0x0000000000fff000) <<  8);//12>>20
        *o |= ((*i & 0x0000000fff000000) << 12);//24>>36
        *o |= ((*i & 0x0000fff000000000) << 16);//36>>52
        o++;
        *o  = ((*i & 0x0fff000000000000) >> 44);//48>>4
        *o |= ((*i & 0xf000000000000000) >> 40);//60>>20
        i++;
        *o |= ((*i & 0x00000000000000ff) << 24);//8>>20
        *o |= ((*i & 0x00000000000fff00) << 28);//8>>36
        *o |= ((*i & 0x00000000fff00000) << 32);//20>>52
        o++;
        *o  = ((*i & 0x00000fff00000000) >> 28);//32>>4
        *o |= ((*i & 0x00fff00000000000) >> 24);//44>>20
        *o |= ((*i & 0xff00000000000000) >> 20);//56>>36
        i++;
        *o |= ((*i & 0x000000000000000f) << 44);//4>>36
        *o |= ((*i & 0x000000000000fff0) << 48);//4>>52
        o++;
        *o  = ((*i & 0x000000000fff0000) >> 12);//16>>4
        *o |= ((*i & 0x000000fff0000000) >>  8);//28>>20
        *o |= ((*i & 0x000fff0000000000) >>  4);//40>>36
        *o |= (*i & 0xfff0000000000000);        //52>>52
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
        out+=8;
        in++;
        noutput_items-=8;
    }
}

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int8_t,56> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint64_t * r = (uint64_t *) buf;
        *p  = (*r & 0x0000000000003fff);        //0>>0
        *p |= ((*r & 0x000000003fff0000) >>  2);//16>>14
        *p |= ((*r & 0x00003fff00000000) >>  4);//32>>28
        *p |= ((*r & 0x3fff000000000000) >>  6);//48>>42
        r++;
        *p |= ((*r & 0x0000000000003fff) << 56);//0>>56
        p++;
        *p  = ((*r & 0x0000000000003fff) >>  8);//0>>-8
        *p |= ((*r & 0x000000003fff0000) >> 10);//16>>6
        *p |= ((*r & 0x00003fff00000000) >> 12);//32>>20
        *p |= ((*r & 0x3fff000000000000) >> 14);//48>>34
        r++;
        *p |= ((*r & 0x0000000000003fff) << 48);//0>>48
        *p |= ((*r & 0x000000003fff0000) << 46);//16>>62
        p++;
        *p  = ((*r & 0x000000003fff0000) >> 18);//16>>-2
        *p |= ((*r & 0x00003fff00000000) >> 20);//32>>12
        *p |= ((*r & 0x3fff000000000000) >> 22);//48>>26
        r++;
        *p |= ((*r & 0x0000000000003fff) << 40);//0>>40
        *p |= ((*r & 0x000000003fff0000) << 38);//16>>54
        p++;
        *p  = ((*r & 0x000000003fff0000) >> 26);//16>>-10
        *p |= ((*r & 0x00003fff00000000) >> 28);//32>>4
        *p |= ((*r & 0x3fff000000000000) >> 30);//48>>18
        r++;
        *p |= ((*r & 0x0000000000003fff) << 32);//0>>32
        *p |= ((*r & 0x000000003fff0000) << 30);//16>>46
        *p |= ((*r & 0x00003fff00000000) << 28);//32>>60
        p++;
        *p  = ((*r & 0x00003fff00000000) >> 36);//32>>-4
        *p |= ((*r & 0x3fff000000000000) >> 38);//48>>10
        r++;
        *p |= ((*r & 0x0000000000003fff) << 24);//0>>24
        *p |= ((*r & 0x000000003fff0000) << 22);//16>>38
        *p |= ((*r & 0x00003fff00000000) << 20);//32>>52
        p++;
        *p  = ((*r & 0x00003fff00000000) >> 44);//32>>-12
        *p |= ((*r & 0x3fff000000000000) >> 46);//48>>2
        r++;
        *p |= ((*r & 0x0000000000003fff) << 16);//0>>16
        *p |= ((*r & 0x000000003fff0000) << 14);//16>>30
        *p |= ((*r & 0x00003fff00000000) << 12);//32>>44
        *p |= ((*r & 0x3fff000000000000) << 10);//48>>58
        p++;
        *p  = ((*r & 0x3fff000000000000) >> 54);//48>>-6
        r++;
        *p |= ((*r & 0x0000000000003fff) <<  8);//0>>8
        *p |= ((*r & 0x000000003fff0000) <<  6);//16>>22
        *p |= ((*r & 0x00003fff00000000) <<  4);//32>>36
        *p |= ((*r & 0x3fff000000000000) <<  2);//48>>50
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int8_t,56> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o  = ((*i & 0x0000000000003fff) <<  2);//0>>2
        *o |= ((*i & 0x000000000fffc000) <<  4);//14>>18
        *o |= ((*i & 0x000003fff0000000) <<  6);//28>>34
        *o |= ((*i & 0x00fffc0000000000) <<  8);//42>>50
        o++;
        *o  = ((*i & 0xff00000000000000) >> 54);//56>>2
        i++;
        *o |= ((*i & 0x000000000000003f) << 10);//6>>2
        *o |= ((*i & 0x00000000000fffc0) << 12);//6>>18
        *o |= ((*i & 0x00000003fff00000) << 14);//20>>34
        *o |= ((*i & 0x0000fffc00000000) << 16);//34>>50
        o++;
        *o  = ((*i & 0x3fff000000000000) >> 46);//48>>2
        *o |= ((*i & 0xc000000000000000) >> 44);//62>>18
        i++;
        *o |= ((*i & 0x0000000000000fff) << 20);//12>>18
        *o |= ((*i & 0x0000000003fff000) << 22);//12>>34
        *o |= ((*i & 0x000000fffc000000) << 24);//26>>50
        o++;
        *o  = ((*i & 0x003fff0000000000) >> 38);//40>>2
        *o |= ((*i & 0xffc0000000000000) >> 36);//54>>18
        i++;
        *o |= ((*i & 0x000000000000000f) << 28);//4>>18
        *o |= ((*i & 0x000000000003fff0) << 30);//4>>34
        *o |= ((*i & 0x00000000fffc0000) << 32);//18>>50
        o++;
        *o  = ((*i & 0x00003fff00000000) >> 30);//32>>2
        *o |= ((*i & 0x0fffc00000000000) >> 28);//46>>18
        *o |= ((*i & 0xf000000000000000) >> 26);//60>>34
        i++;
        *o |= ((*i & 0x00000000000003ff) << 38);//10>>34
        *o |= ((*i & 0x0000000000fffc00) << 40);//10>>50
        o++;
        *o  = ((*i & 0x0000003fff000000) >> 22);//24>>2
        *o |= ((*i & 0x000fffc000000000) >> 20);//38>>18
        *o |= ((*i & 0xfff0000000000000) >> 18);//52>>34
        i++;
        *o |= ((*i & 0x0000000000000003) << 46);//2>>34
        *o |= ((*i & 0x000000000000fffc) << 48);//2>>50
        o++;
        *o  = ((*i & 0x000000003fff0000) >> 14);//16>>2
        *o |= ((*i & 0x00000fffc0000000) >> 12);//30>>18
        *o |= ((*i & 0x03fff00000000000) >> 10);//44>>34
        *o |= ((*i & 0xfc00000000000000) >>  8);//58>>50
        i++;
        *o |= ((*i & 0x00000000000000ff) << 56);//8>>50
        o++;
        *o  = ((*i & 0x00000000003fff00) >>  6);//8>>2
        *o |= ((*i & 0x0000000fffc00000) >>  4);//22>>18
        *o |= ((*i & 0x0003fff000000000) >>  2);//36>>34
        *o |= (*i & 0xfffc000000000000);        //50>>50

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        in++;
        noutput_items-=16;
    }
}

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int16_t,20> * out, int noutput_items)
{
    uint8_t buf[32*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 64);
        uint64_t * r = (uint64_t *) buf;
        *p  = (*r & 0x00000000000003ff);        //0>>0
        *p |= ((*r & 0x000003ff00000000) >> 22);//32>>10
        r++;
        *p |= ((*r & 0x00000000000003ff) << 20);//0>>20
        *p |= ((*r & 0x000003ff00000000) >>  2);//32>>30
        r++;
        *p |= ((*r & 0x00000000000003ff) << 40);//0>>40
        *p |= ((*r & 0x000003ff00000000) << 18);//32>>50
        r++;
        *p |= ((*r & 0x00000000000003ff) << 60);//0>>60
        p++;
        *p  = ((*r & 0x00000000000003ff) >>  4);//0>>-4
        *p |= ((*r & 0x000003ff00000000) >> 26);//32>>6
        r++;
        *p |= ((*r & 0x00000000000003ff) << 16);//0>>16
        *p |= ((*r & 0x000003ff00000000) >>  6);//32>>26
        r++;
        *p |= ((*r & 0x00000000000003ff) << 36);//0>>36
        *p |= ((*r & 0x000003ff00000000) << 14);//32>>46
        r++;
        *p |= ((*r & 0x00000000000003ff) << 56);//0>>56
        p++;
        *p  = ((*r & 0x00000000000003ff) >>  8);//0>>-8
        *p |= ((*r & 0x000003ff00000000) >> 30);//32>>2
        r++;
        *p |= ((*r & 0x00000000000003ff) << 12);//0>>12
        *p |= ((*r & 0x000003ff00000000) >> 10);//32>>22
        r++;
        *p |= ((*r & 0x00000000000003ff) << 32);//0>>32
        *p |= ((*r & 0x000003ff00000000) << 10);//32>>42
        r++;
        *p |= ((*r & 0x00000000000003ff) << 52);//0>>52
        *p |= ((*r & 0x000003ff00000000) << 30);//32>>62
        p++;
        *p  = ((*r & 0x000003ff00000000) >> 34);//32>>-2
        r++;
        *p |= ((*r & 0x00000000000003ff) <<  8);//0>>8
        *p |= ((*r & 0x000003ff00000000) >> 14);//32>>18
        r++;
        *p |= ((*r & 0x00000000000003ff) << 28);//0>>28
        *p |= ((*r & 0x000003ff00000000) <<  6);//32>>38
        r++;
        *p |= ((*r & 0x00000000000003ff) << 48);//0>>48
        *p |= ((*r & 0x000003ff00000000) << 26);//32>>58
        p++;
        *p  = ((*r & 0x000003ff00000000) >> 38);//32>>-6
        r++;
        *p |= ((*r & 0x00000000000003ff) <<  4);//0>>4
        *p |= ((*r & 0x000003ff00000000) >> 18);//32>>14
        r++;
        *p |= ((*r & 0x00000000000003ff) << 24);//0>>24
        *p |= ((*r & 0x000003ff00000000) <<  2);//32>>34
        r++;
        *p |= ((*r & 0x00000000000003ff) << 44);//0>>44
        *p |= ((*r & 0x000003ff00000000) << 22);//32>>54
        p++;

        in+=32;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int16_t,20> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[32*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o  = ((*i & 0x00000000000003ff) <<  6);//0>>6
        *o |= ((*i & 0x00000000000ffc00) << 28);//10>>38
        o++;
        *o  = ((*i & 0x000000003ff00000) >> 14);//20>>6
        *o |= ((*i & 0x000000ffc0000000) <<  8);//30>>38
        o++;
        *o  = ((*i & 0x0003ff0000000000) >> 34);//40>>6
        *o |= ((*i & 0x0ffc000000000000) >> 12);//50>>38
        o++;
        *o  = ((*i & 0xf000000000000000) >> 54);//60>>6
        i++;
        *o |= ((*i & 0x000000000000003f) << 10);//6>>6
        *o |= ((*i & 0x000000000000ffc0) << 32);//6>>38
        o++;
        *o  = ((*i & 0x0000000003ff0000) >> 10);//16>>6
        *o |= ((*i & 0x0000000ffc000000) << 12);//26>>38
        o++;
        *o  = ((*i & 0x00003ff000000000) >> 30);//36>>6
        *o |= ((*i & 0x00ffc00000000000) >>  8);//46>>38
        o++;
        *o  = ((*i & 0xff00000000000000) >> 50);//56>>6
        i++;
        *o |= ((*i & 0x0000000000000003) << 14);//2>>6
        *o |= ((*i & 0x0000000000000ffc) << 36);//2>>38
        o++;
        *o  = ((*i & 0x00000000003ff000) >>  6);//12>>6
        *o |= ((*i & 0x00000000ffc00000) << 16);//22>>38
        o++;
        *o  = ((*i & 0x000003ff00000000) >> 26);//32>>6
        *o |= ((*i & 0x000ffc0000000000) >>  4);//42>>38
        o++;
        *o  = ((*i & 0x3ff0000000000000) >> 46);//52>>6
        *o |= ((*i & 0xc000000000000000) >> 24);//62>>38
        i++;
        *o |= ((*i & 0x00000000000000ff) << 40);//8>>38
        o++;
        *o  = ((*i & 0x000000000003ff00) >>  2);//8>>6
        *o |= ((*i & 0x000000000ffc0000) << 20);//18>>38
        o++;
        *o  = ((*i & 0x0000003ff0000000) >> 22);//28>>6
        *o |= (*i & 0x0000ffc000000000);        //38>>38
        o++;
        *o  = ((*i & 0x03ff000000000000) >> 42);//48>>6
        *o |= ((*i & 0xfc00000000000000) >> 20);//58>>38
        i++;
        *o |= ((*i & 0x000000000000000f) << 44);//4>>38
        o++;
        *o  = ((*i & 0x0000000000003ff0) <<  2);//4>>6
        *o |= ((*i & 0x0000000000ffc000) << 24);//14>>38
        o++;
        *o  = ((*i & 0x00000003ff000000) >> 18);//24>>6
        *o |= ((*i & 0x00000ffc00000000) <<  4);//34>>38
        o++;
        *o  = ((*i & 0x003ff00000000000) >> 38);//44>>6
        *o |= ((*i & 0xffc0000000000000) >> 16);//54>>38
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,64);
        out+=32;
        in++;
        noutput_items-=32;
    }
}

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int16_t,12> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint64_t * r = (uint64_t *) buf;
        *p  = (*r & 0x0000000000000fff);        //0>>0
        *p |= ((*r & 0x00000fff00000000) >> 20);//32>>12
        r++;
        *p |= ((*r & 0x0000000000000fff) << 24);//0>>24
        *p |= ((*r & 0x00000fff00000000) <<  4);//32>>36
        r++;
        *p |= ((*r & 0x0000000000000fff) << 48);//0>>48
        *p |= ((*r & 0x00000fff00000000) << 28);//32>>60
        p++;
        *p  = ((*r & 0x00000fff00000000) >> 36);//32>>-4
        r++;
        *p |= ((*r & 0x0000000000000fff) <<  8);//0>>8
        *p |= ((*r & 0x00000fff00000000) >> 12);//32>>20
        r++;
        *p |= ((*r & 0x0000000000000fff) << 32);//0>>32
        *p |= ((*r & 0x00000fff00000000) << 12);//32>>44
        r++;
        *p |= ((*r & 0x0000000000000fff) << 56);//0>>56
        p++;
        *p  = ((*r & 0x0000000000000fff) >>  8);//0>>-8
        *p |= ((*r & 0x00000fff00000000) >> 28);//32>>4
        r++;
        *p |= ((*r & 0x0000000000000fff) << 16);//0>>16
        *p |= ((*r & 0x00000fff00000000) >>  4);//32>>28
        r++;
        *p |= ((*r & 0x0000000000000fff) << 40);//0>>40
        *p |= ((*r & 0x00000fff00000000) << 20);//32>>52
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int16_t,12> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o  = ((*i & 0x0000000000000fff) <<  4);//0>>4
        *o |= ((*i & 0x0000000000fff000) << 24);//12>>36
        o++;
        *o  = ((*i & 0x0000000fff000000) >> 20);//24>>4
        *o |= (*i & 0x0000fff000000000);        //36>>36
        o++;
        *o  = ((*i & 0x0fff000000000000) >> 44);//48>>4
        *o |= ((*i & 0xf000000000000000) >> 24);//60>>36
        i++;
        *o |= ((*i & 0x00000000000000ff) << 40);//8>>36
        o++;
        *o  = ((*i & 0x00000000000fff00) >>  4);//8>>4
        *o |= ((*i & 0x00000000fff00000) << 16);//20>>36
        o++;
        *o  = ((*i & 0x00000fff00000000) >> 28);//32>>4
        *o |= ((*i & 0x00fff00000000000) >>  8);//44>>36
        o++;
        *o  = ((*i & 0xff00000000000000) >> 52);//56>>4
        i++;
        *o |= ((*i & 0x000000000000000f) << 12);//4>>4
        *o |= ((*i & 0x000000000000fff0) << 32);//4>>36
        o++;
        *o  = ((*i & 0x000000000fff0000) >> 12);//16>>4
        *o |= ((*i & 0x000000fff0000000) <<  8);//28>>36
        o++;
        *o  = ((*i & 0x000fff0000000000) >> 36);//40>>4
        *o |= ((*i & 0xfff0000000000000) >> 16);//52>>36
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        in++;
        noutput_items-=16;
    }
}

void any_to_any_impl_64::convert(const gr_complex *in, std::array<int16_t,28> * out, int noutput_items)
{
    uint8_t buf[32*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 64);
        uint64_t * r = (uint64_t *) buf;
        *p  = (*r & 0x0000000000003fff);        //0>>0
        *p |= ((*r & 0x00003fff00000000) >> 18);//32>>14
        r++;
        *p |= ((*r & 0x0000000000003fff) << 28);//0>>28
        *p |= ((*r & 0x00003fff00000000) << 10);//32>>42
        r++;
        *p |= ((*r & 0x0000000000003fff) << 56);//0>>56
        p++;
        *p  = ((*r & 0x0000000000003fff) >>  8);//0>>-8
        *p |= ((*r & 0x00003fff00000000) >> 26);//32>>6
        r++;
        *p |= ((*r & 0x0000000000003fff) << 20);//0>>20
        *p |= ((*r & 0x00003fff00000000) <<  2);//32>>34
        r++;
        *p |= ((*r & 0x0000000000003fff) << 48);//0>>48
        *p |= ((*r & 0x00003fff00000000) << 30);//32>>62
        p++;
        *p  = ((*r & 0x00003fff00000000) >> 34);//32>>-2
        r++;
        *p |= ((*r & 0x0000000000003fff) << 12);//0>>12
        *p |= ((*r & 0x00003fff00000000) >>  6);//32>>26
        r++;
        *p |= ((*r & 0x0000000000003fff) << 40);//0>>40
        *p |= ((*r & 0x00003fff00000000) << 22);//32>>54
        p++;
        *p  = ((*r & 0x00003fff00000000) >> 42);//32>>-10
        r++;
        *p |= ((*r & 0x0000000000003fff) <<  4);//0>>4
        *p |= ((*r & 0x00003fff00000000) >> 14);//32>>18
        r++;
        *p |= ((*r & 0x0000000000003fff) << 32);//0>>32
        *p |= ((*r & 0x00003fff00000000) << 14);//32>>46
        r++;
        *p |= ((*r & 0x0000000000003fff) << 60);//0>>60
        p++;
        *p  = ((*r & 0x0000000000003fff) >>  4);//0>>-4
        *p |= ((*r & 0x00003fff00000000) >> 22);//32>>10
        r++;
        *p |= ((*r & 0x0000000000003fff) << 24);//0>>24
        *p |= ((*r & 0x00003fff00000000) <<  6);//32>>38
        r++;
        *p |= ((*r & 0x0000000000003fff) << 52);//0>>52
        p++;
        *p  = ((*r & 0x0000000000003fff) >> 12);//0>>-12
        *p |= ((*r & 0x00003fff00000000) >> 30);//32>>2
        r++;
        *p |= ((*r & 0x0000000000003fff) << 16);//0>>16
        *p |= ((*r & 0x00003fff00000000) >>  2);//32>>30
        r++;
        *p |= ((*r & 0x0000000000003fff) << 44);//0>>44
        *p |= ((*r & 0x00003fff00000000) << 26);//32>>58
        p++;
        *p  = ((*r & 0x00003fff00000000) >> 38);//32>>-6
        r++;
        *p |= ((*r & 0x0000000000003fff) <<  8);//0>>8
        *p |= ((*r & 0x00003fff00000000) >> 10);//32>>22
        r++;
        *p |= ((*r & 0x0000000000003fff) << 36);//0>>36
        *p |= ((*r & 0x00003fff00000000) << 18);//32>>50
        p++;

        in+=32;
        noutput_items--;
    }
}

void any_to_any_impl_64::convert(const std::array<int16_t,28> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[32*2*2];
    noutput_items&=-2;
    while(noutput_items)
    {
        uint64_t * i = (uint64_t *) &(*in)[0];
        uint64_t * o = (uint64_t *) buf;
        *o  = ((*i & 0x0000000000003fff) <<  2);//0>>2
        *o |= ((*i & 0x000000000fffc000) << 20);//14>>34
        o++;
        *o  = ((*i & 0x000003fff0000000) >> 26);//28>>2
        *o |= ((*i & 0x00fffc0000000000) >>  8);//42>>34
        o++;
        *o  = ((*i & 0xff00000000000000) >> 54);//56>>2
        i++;
        *o |= ((*i & 0x000000000000003f) << 10);//6>>2
        *o |= ((*i & 0x00000000000fffc0) << 28);//6>>34
        o++;
        *o  = ((*i & 0x00000003fff00000) >> 18);//20>>2
        *o |= (*i & 0x0000fffc00000000);        //34>>34
        o++;
        *o  = ((*i & 0x3fff000000000000) >> 46);//48>>2
        *o |= ((*i & 0xc000000000000000) >> 28);//62>>34
        i++;
        *o |= ((*i & 0x0000000000000fff) << 36);//12>>34
        o++;
        *o  = ((*i & 0x0000000003fff000) >> 10);//12>>2
        *o |= ((*i & 0x000000fffc000000) <<  8);//26>>34
        o++;
        *o  = ((*i & 0x003fff0000000000) >> 38);//40>>2
        *o |= ((*i & 0xffc0000000000000) >> 20);//54>>34
        i++;
        *o |= ((*i & 0x000000000000000f) << 44);//4>>34
        o++;
        *o  = ((*i & 0x000000000003fff0) >>  2);//4>>2
        *o |= ((*i & 0x00000000fffc0000) << 16);//18>>34
        o++;
        *o  = ((*i & 0x00003fff00000000) >> 30);//32>>2
        *o |= ((*i & 0x0fffc00000000000) >> 12);//46>>34
        o++;
        *o  = ((*i & 0xf000000000000000) >> 58);//60>>2
        i++;
        *o |= ((*i & 0x00000000000003ff) <<  6);//10>>2
        *o |= ((*i & 0x0000000000fffc00) << 24);//10>>34
        o++;
        *o  = ((*i & 0x0000003fff000000) >> 22);//24>>2
        *o |= ((*i & 0x000fffc000000000) >>  4);//38>>34
        o++;
        *o  = ((*i & 0xfff0000000000000) >> 50);//52>>2
        i++;
        *o |= ((*i & 0x0000000000000003) << 14);//2>>2
        *o |= ((*i & 0x000000000000fffc) << 32);//2>>34
        o++;
        *o  = ((*i & 0x000000003fff0000) >> 14);//16>>2
        *o |= ((*i & 0x00000fffc0000000) <<  4);//30>>34
        o++;
        *o  = ((*i & 0x03fff00000000000) >> 42);//44>>2
        *o |= ((*i & 0xfc00000000000000) >> 24);//58>>34
        i++;
        *o |= ((*i & 0x00000000000000ff) << 40);//8>>34
        o++;
        *o  = ((*i & 0x00000000003fff00) >>  6);//8>>2
        *o |= ((*i & 0x0000000fffc00000) << 12);//22>>34
        o++;
        *o  = ((*i & 0x0003fff000000000) >> 34);//36>>2
        *o |= ((*i & 0xfffc000000000000) >> 16);//50>>34
        i++;

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,64);
        out+=32;
        in++;
        noutput_items-=32;
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

void any_to_any_impl_bmi64::convert(const gr_complex *in, std::array<int8_t,24> * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
        uint64_t * r = (uint64_t *) buf;
        uint64_t t;
        uint64_t b;
        b=_pext_u64(*r,0x0fff0fff0fff0fff);//48
        r++;
        t=_pext_u64(*r,0x0fff0fff0fff0fff);//96
        r++;
        b|=t<<48;

        *p=b;//32
        p++;

        b=_pext_u64(*r,0x0fff0fff0fff0fff);//80
        r++;
        t=t>>16;//64-48=16
        t|=b<<32;//48-16=32
        *p=t;//16
        p++;

        t=_pext_u64(*r,0x0fff0fff0fff0fff);//64
        r++;
        b=b>>32;//64-32=32
        b|=t<<16;//48-32=16

        *p=b;//0
        p++;

        in+=8;
        noutput_items--;
    }
}

void any_to_any_impl_bmi64::convert(const std::array<int8_t,24> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[8*2*2];
    uint64_t t;
    uint64_t * i = (uint64_t *) &(*in)[0];
    while(noutput_items)
    {
        uint64_t * o = (uint64_t *) buf;
        *o = _pdep_u64(*i, 0xfff0fff0fff0fff0);//16
        o++;
        t = *i >> 48;
        i++;
        t |= *i << 16;
        *o = _pdep_u64(t, 0xfff0fff0fff0fff0);
        o++;
        t = *i >> 32;
        i++;
        t |= *i << 32;
        *o = _pdep_u64(t, 0xfff0fff0fff0fff0);
        o++;
        t = *i >> 16;
        i++;
        *o = _pdep_u64(t, 0xfff0fff0fff0fff0);
        o++;
        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
        out+=8;
        noutput_items-=8;
    }
}

void any_to_any_impl_bmi64::convert(const gr_complex *in, std::array<int8_t,56> * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    uint64_t * p = (uint64_t *) &(*out)[0];
    constexpr uint64_t mask=0x3fff3fff3fff3fff;
    while(noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
        uint64_t * r = (uint64_t *) buf;
        uint64_t t;
        uint64_t b;
        b=_pext_u64(*r,mask);
        r++;
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<56;

        *p=b;
        p++;

        b=t>>8;//64-56=8
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<48;//56-8=48

        *p=b;
        p++;

        b=t>>16;//64-48=16
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<40;//56-16=40

        *p=b;
        p++;

        b=t>>24;//64-40=24
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<32;//56-24=32

        *p=b;
        p++;

        b=t>>32;//64-32=32
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<24;//56-32=24

        *p=b;
        p++;

        b=t>>40;//64-24=40
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<16;//56-40=16

        *p=b;
        p++;

        b=t>>48;//64-16=48
        t=_pext_u64(*r,mask);
        r++;
        b|=t<<8;//56-48=8

        *p=b;
        p++;

        in+=16;
        noutput_items--;
    }
}

void any_to_any_impl_bmi64::convert(const std::array<int8_t,56> *in, gr_complex * out, int noutput_items)
{
    uint8_t buf[16*2*2];
    noutput_items&=-2;
    constexpr uint64_t mask=0xfffcfffcfffcfffc;
    uint64_t t;
    uint64_t * i = (uint64_t *) &(*in)[0];
    while(noutput_items)
    {
        uint64_t * o = (uint64_t *) buf;
        *o = _pdep_u64(*i, mask);
        o++;

        t = *i >> 56;
        i++;
        t |= *i << 8;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 48;
        i++;
        t |= *i << 16;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 40;
        i++;
        t |= *i << 24;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 32;
        i++;
        t |= *i << 32;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 24;
        i++;
        t |= *i << 40;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 16;
        i++;
        t |= *i << 48;
        *o = _pdep_u64(t, mask);
        o++;

        t = *i >> 8;
        i++;
        *o = _pdep_u64(t, mask);

        volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
        out+=16;
        noutput_items-=16;
    }
}

#endif

constexpr std::array<any_to_any_base::format_descriptor, FILE_FORMAT_COUNT> any_to_any_base::fmt;
