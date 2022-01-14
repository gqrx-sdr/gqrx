//////////////////////////////////////////////////////////////////////
// agc_impl.cpp: implementation of the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//      2011-09-24  Adapted for gqrx
//      2022-01-10  Rewritten with low complexity alogo
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================

#include <dsp/agc_impl.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <volk/volk.h>

#define AGC_DEBUG

#define MIN_GAIN_DB (-20.0f)
#define MIN_GAIN powf(10.0, MIN_GAIN_DB)

CAgc::CAgc():
    d_sample_rate(0),
    d_agc_on(false),
    d_target_level(0),
    d_manual_gain(0),
    d_max_gain(-1),
    d_attack(0),
    d_decay(0),
    d_hang(0),
    d_target_mag(1),
    d_buf_size(0),
    d_buf_p(0)
{
    SetParameters(2000, 0, 0, 0, 100, 50, 50, 0, true);
}

CAgc::~CAgc()
{

}

void CAgc::SetParameters(double sample_rate, bool agc_on, int target_level,
                         float manual_gain, int max_gain, int attack, int decay,
                         int hang, bool force)
{
    bool samp_rate_changed = false;
    bool agc_on_changed = false;
    bool target_level_changed = false;
    bool manual_gain_changed = false;
    bool max_gain_changed = false;
    bool attack_changed = false;
    bool decay_changed = false;
    bool hang_changed = false;
    if (d_sample_rate != sample_rate || force)
    {
        d_sample_rate = sample_rate;
        samp_rate_changed = true;
    }
    if (d_agc_on != agc_on  || force)
    {
        d_agc_on = agc_on;
        agc_on_changed = true;
    }
    if (d_target_level != target_level || force)
    {
        d_target_level = target_level;
        d_target_mag = powf(10.0, TYPEFLOAT(d_target_level) / 20.0);
        target_level_changed = true;
    }
    if (d_manual_gain != manual_gain || force)
    {
        d_manual_gain = manual_gain;
        manual_gain_changed = true;
    }
    if (d_max_gain != max_gain || force)
    {
        d_max_gain = max_gain;
        if(d_max_gain < 1)
            d_max_gain = 1;
        d_max_gain_mag = powf(10.0, d_max_gain / 20.0);
        max_gain_changed = true;
    }
    if (d_attack != attack || force)
    {
        d_attack = attack;
        attack_changed = true;
    }
    if (d_decay != decay || force)
    {
        d_decay = decay;
        decay_changed = true;
    }
    if (d_hang != hang || force)
    {
        d_hang = hang;
        hang_changed = true;
    }
    if (samp_rate_changed || attack_changed)
    {
        d_buf_samples = sample_rate * d_attack / 1000.0;
        int buf_size = 1;
        for(unsigned int k = 0; k < sizeof(int) * 8; k++)
        {
            buf_size *= 2;
            if(buf_size >= d_buf_samples)
                break;
        }
        if (d_buf_p >= d_buf_samples)
            d_buf_p %= d_buf_samples;
        if(d_buf_size != buf_size)
        {
            d_buf_size = buf_size;
            d_sample_buf.clear();
            d_sample_buf.resize(d_buf_size, 0);
            d_mag_buf.clear();
            d_mag_buf.resize(d_buf_size * 2, 0);
            d_buf_p = 0;
            d_max_idx = d_buf_size * 2 - 2;
         }
    }
    if ((manual_gain_changed || agc_on_changed) && !agc_on)
        d_current_gain = powf(10.0, TYPEFLOAT(d_manual_gain) / 20.0);
    if (max_gain_changed || attack_changed || samp_rate_changed)
        d_attack_step = 1.0 / powf(10.0, std::max(TYPEFLOAT(d_max_gain), - MIN_GAIN_DB) / TYPEFLOAT(d_buf_samples) / 20.0);
    if (max_gain_changed || decay_changed || samp_rate_changed)
        d_decay_step = powf(10.0, TYPEFLOAT(d_max_gain) / TYPEFLOAT(sample_rate * d_decay / 1000.0) / 20.0);
    if (hang_changed || samp_rate_changed)
        d_hang_samp = sample_rate * d_hang / 1000.0;

    if (target_level_changed || max_gain_changed)
        d_floor = powf(10.0, TYPEFLOAT(d_target_level - d_max_gain) / 20.0);
    #ifdef AGC_DEBUG
    std::cerr<<std::endl<<
        "d_target_mag="<<d_target_mag<<std::endl<<
        "d_target_level="<<d_target_level<<std::endl<<
        "d_manual_gain="<<d_manual_gain<<std::endl<<
        "d_max_gain="<<d_max_gain<<std::endl<<
        "d_attack="<<d_attack<<std::endl<<
        "d_attack_step="<<log10(d_attack_step)*20.0<<std::endl<<
        "d_decay="<<d_decay<<std::endl<<
        "d_decay_step="<<log10(d_decay_step)*20.0<<std::endl<<
        "d_hang="<<d_hang<<std::endl<<
        "d_hang_samp="<<d_hang_samp<<std::endl<<
        "d_buf_samples="<<d_buf_samples<<std::endl<<
        "d_buf_size="<<d_buf_size<<std::endl<<
        "";
    #endif
}

float CAgc::get_peak()
{
    return d_mag_buf[d_max_idx];
}

void CAgc::update_buffer(int p)
{
    int ofs = 0;
    int base = d_buf_size;
    while (base > 1)
    {
        float max_p = std::max(d_mag_buf[ofs + p], d_mag_buf[ofs + (p ^ 1)]);
        p = p >> 1;
        ofs += base;
        if(d_mag_buf[ofs + p] != max_p)
            d_mag_buf[ofs + p] = max_p;
        else
            break;
        base = base >> 1;
    }

}

void CAgc::ProcessData(float * pOutData0,float * pOutData1, const float * pInData0, const float * pInData1, int Length)
{
    int k;
    TYPEFLOAT max_out = 0;
    TYPEFLOAT mag_in = 0;
    if (d_agc_on)
    {
//        float * mag_vect = &((float *)pOutData)[Length];
//        volk_32fc_magnitude_32f(mag_vect, pInData, Length);
        for (k = 0; k < Length; k++)
        {
            float sample_in0 = pInData0[k];
            float sample_in1 = pInData1[k];
//            mag_in = mag_vect[k];
            mag_in = std::max(fabs(sample_in0),fabs(sample_in1));
            float sample_out0 = d_sample_buf[d_buf_p].real();
            float sample_out1 = d_sample_buf[d_buf_p].imag();

            d_sample_buf[d_buf_p] = TYPECPX(sample_in0, sample_in1);
            d_mag_buf[d_buf_p] = mag_in;
            update_buffer(d_buf_p);
            max_out = get_peak();

            int buf_p_next = d_buf_p + 1;
            if (buf_p_next >= d_buf_samples)
                buf_p_next = 0;

            if (max_out > d_floor)
            {
                float new_target = d_target_mag / max_out;
                if (new_target < d_target_gain)
                {
                    if (d_current_gain > d_target_gain)
                        d_hang_counter = d_buf_samples + d_hang_samp;
                    d_target_gain = new_target;
                }
                else
                    if (!d_hang_counter)
                        d_target_gain = new_target;
            }
            else
            {
                d_target_gain = d_max_gain_mag;
                d_hang_counter = 0;
            }
            if (d_current_gain > d_target_gain)
            {
                //attack, decrease gain one step per sample
                d_current_gain *= d_attack_step;
            }
            else
            {
                if (d_hang_counter <= 0)
                {
                    //decay, increase gain one step per sample until we reach d_max_gain
                    if (d_current_gain < d_target_gain)
                        d_current_gain *= d_decay_step;
                    if (d_current_gain > d_target_gain)
                        d_current_gain = d_target_gain;
                }
            }
            if (d_hang_counter > 0)
                d_hang_counter--;
            if (d_current_gain < MIN_GAIN)
                d_current_gain = MIN_GAIN;
            pOutData0[k] = sample_out0 * d_current_gain;
            pOutData1[k] = sample_out1 * d_current_gain;
            d_buf_p = buf_p_next;
        }
    }
    else{
        volk_32f_s32f_multiply_32f((float *)pOutData0, (float *)pInData0, d_current_gain, Length);
        volk_32f_s32f_multiply_32f((float *)pOutData1, (float *)pInData1, d_current_gain, Length);
    }
    #ifdef AGC_DEBUG2
    if(d_prev_dbg != d_target_gain)
    {
        std::cerr<<"------ d_target_gain="<<d_target_gain<<" d_current_gain="<<d_current_gain<<" d_hang_counter="<<d_hang_counter<<  std::endl;
        d_prev_dbg = d_target_gain;
    }
    #endif
}

float CAgc::CurrentGainDb()
{
    return 20.0 * log10f(d_current_gain);
}
