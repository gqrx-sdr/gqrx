//////////////////////////////////////////////////////////////////////
// agc_impl.cpp: implementation of the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//      2011-09-24  Adapted for gqrx
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

//////////////////////////////////////////////////////////////////////
// Local Defines
//////////////////////////////////////////////////////////////////////

//signal delay line time delay in seconds.
//adjust to cover the impulse response time of filter
#define DELAY_TIMECONST .015

//Peak Detector window time delay in seconds.
#define WINDOW_TIMECONST .018

//attack time constant in seconds
//just small enough to let attackave charge up within the DELAY_TIMECONST time
#define ATTACK_RISE_TIMECONST .002
#define ATTACK_FALL_TIMECONST .005

#define DECAY_RISEFALL_RATIO .3	//ratio between rise and fall times of Decay time constants
//adjust for best action with SSB

// hang timer release decay time constant in seconds
#define RELEASE_TIMECONST .05

//limit output to about 3db of max
#define AGC_OUTSCALE 0.7

// keep max in and out the same
#define MAX_AMPLITUDE 1.0 //32767.0
#define MAX_MANUAL_AMPLITUDE 1.0 //32767.0

#define LOG_MAX_AMPL    log10f(MAX_AMPLITUDE)

#define MIN_CONSTANT 1e-8   // const for calc log() so that a value of 0 magnitude == -8
                            // corresponding to -160dB.
                            // K = 10^(-8 + log(MAX_AMP))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAgc::CAgc()
{
    m_AgcOn = true;
    m_UseHang = false;
    m_Threshold = 0;
    m_ManualGain = 0;
    m_SlopeFactor = 0;
    m_Decay = 0;
    m_SampleRate = 100.0;
    m_SigDelayBuf_r = (float*)(&m_SigDelayBuf);
    m_ManualAgcGain = 0.f;
    m_DecayAve = 0.f;
    m_AttackAve = 0.f;
    m_AttackRiseAlpha = 0.f;
    m_AttackFallAlpha = 0.f;
    m_DecayRiseAlpha = 0.f;
    m_DecayFallAlpha = 0.f;
    m_FixedGain = 0.f;
    m_Knee = 0.f;
    m_GainSlope = 0.f;
    m_Peak = 0.f;
    m_SigDelayPtr = 0;
    m_MagBufPos = 0;
    m_DelaySamples = 0;
    m_WindowSamples = 0;
    m_HangTime = 0;
    m_HangTimer = 0;
}

CAgc::~CAgc()
{
}

////////////////////////////////////////////////////////////////////////////////
// Sets and calculates various AGC parameters
//  "On"  switches between AGC on and off.
//  "Threshold" specifies AGC Knee in dB if AGC is active.( nominal range -160 to 0dB)
//  "ManualGain" specifies AGC manual gain in dB if AGC is not active.(nominal range 0 to 100dB)
//  "SlopeFactor" specifies dB reduction in output at knee from maximum output level(nominal range 0 to 10dB)
//  "Decay" is AGC decay value in milliseconds ( nominal range 20 to 5000 milliSeconds)
//  "SampleRate" is current sample rate of AGC data
////////////////////////////////////////////////////////////////////////////////
void CAgc::SetParameters(bool AgcOn,  bool UseHang, int Threshold, int ManualGain,
                         int SlopeFactor, int Decay, double SampleRate)
{
    if((AgcOn == m_AgcOn) && (UseHang == m_UseHang) &&
            (Threshold == m_Threshold) && (ManualGain == m_ManualGain) &&
            (SlopeFactor == m_SlopeFactor) && (Decay == m_Decay) &&
            (SampleRate == m_SampleRate))
    {
        return;		//just return if no parameter changed
    }

    m_AgcOn = AgcOn;
    m_UseHang = UseHang;
    m_Threshold = Threshold;
    m_ManualGain = ManualGain;
    m_SlopeFactor = SlopeFactor;
    m_Decay = Decay;
    if (m_SampleRate != SampleRate)
    {
        //clear out delay buffer and init some things if sample rate changes
        m_SampleRate = SampleRate;
        for (int i = 0; i < MAX_DELAY_BUF; i++)
        {
            m_SigDelayBuf[i] = 0.0;
            m_MagBuf[i] = -16.0;
        }
        m_SigDelayPtr = 0;
        m_HangTimer = 0;
        m_Peak = -16.0;
        m_DecayAve = -5.0;
        m_AttackAve = -5.0;
        m_MagBufPos = 0;
    }

    // convert m_ThreshGain to linear manual gain value
    m_ManualAgcGain = MAX_MANUAL_AMPLITUDE * powf(10.0, (float)m_ManualGain / 20.0);

    // calculate parameters for AGC gain as a function of input magnitude
    m_Knee = (float)m_Threshold / 20.0;
    m_GainSlope = m_SlopeFactor / 100.0;

    // fixed gain value used below knee threshold
    m_FixedGain = AGC_OUTSCALE * powf(10.0, m_Knee * (m_GainSlope - 1.0) );

    // calculate fast and slow filter values.
    m_AttackRiseAlpha = (1.0 - expf(-1.0 / (m_SampleRate * ATTACK_RISE_TIMECONST)));
    m_AttackFallAlpha = (1.0 - expf(-1.0 / (m_SampleRate * ATTACK_FALL_TIMECONST)));

    // make rise time DECAY_RISEFALL_RATIO of fall
    m_DecayRiseAlpha = (1.0 - expf(-1.0 / (m_SampleRate * (float)m_Decay * 0.001 * DECAY_RISEFALL_RATIO)));
    m_HangTime = (int)(m_SampleRate * (float)m_Decay * .001);

    if (m_UseHang)
        m_DecayFallAlpha = (1.0 - expf(-1.0 / (m_SampleRate * RELEASE_TIMECONST)));
    else
        m_DecayFallAlpha = (1.0 - expf(-1.0 / (m_SampleRate * (float)m_Decay * 0.001)));

    m_DelaySamples = (int)(m_SampleRate * DELAY_TIMECONST);
    m_WindowSamples = (int)(m_SampleRate * WINDOW_TIMECONST);

    // clamp Delay samples within buffer limit
    if (m_DelaySamples >= MAX_DELAY_BUF - 1)
        m_DelaySamples = MAX_DELAY_BUF - 1;
}



//////////////////////////////////////////////////////////////////////
// Automatic Gain Control calculator for COMPLEX data
//////////////////////////////////////////////////////////////////////
void CAgc::ProcessData(int Length, const TYPECPX * pInData, TYPECPX * pOutData)
{
    float       gain;
    float       mag;
    TYPECPX     delayedin;

    if (m_AgcOn)
    {
        for (int i = 0; i < Length; i++)
        {
            // get latest input sample
            TYPECPX     in = pInData[i];

            // Get delayed sample of input signal
            delayedin = m_SigDelayBuf[m_SigDelayPtr];

            // put new input sample into signal delay buffer
            m_SigDelayBuf[m_SigDelayPtr++] = in;

            // deal with delay buffer wrap around
            if (m_SigDelayPtr >= m_DelaySamples)
                m_SigDelayPtr = 0;

            mag = fabs(in.real());
            float mim = fabs(in.imag());
            if (mim > mag)
                mag = mim;
            mag = log10f(mag + MIN_CONSTANT) - LOG_MAX_AMPL;

            // create a sliding window of 'm_WindowSamples' magnitudes and output the peak value within the sliding window
            float tmp = m_MagBuf[m_MagBufPos];   // get oldest mag sample from buffer into tmp
            m_MagBuf[m_MagBufPos++] = mag;       // put latest mag sample in buffer;
            if (m_MagBufPos >= m_WindowSamples)  // deal with magnitude buffer wrap around
                m_MagBufPos = 0;
            if (mag > m_Peak)
            {
                m_Peak = mag;	//if new sample is larger than current peak then use it, no need to look at buffer values
            }
            else
            {
                if (tmp == m_Peak)    //tmp is oldest sample pulled out of buffer
                {                     //if oldest sample pulled out was last peak then need to find next highest peak in buffer
                    m_Peak = -8.0;    //set to lowest value to find next max peak
                    //search all buffer for maximum value and set as new peak
                    for (int i = 0; i < m_WindowSamples; i++)
                    {
                        tmp = m_MagBuf[i];
                        if (tmp > m_Peak)
                            m_Peak = tmp;
                    }
                }
            }

            if (m_UseHang)
            {
                // using hang timer mode
                if (m_Peak > m_AttackAve)
                    // if power is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                {
                    // if magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                  m_DecayRiseAlpha * m_Peak;
                    // reset hang timer
                    m_HangTimer = 0;
                }
                else
                {	// here if decreasing signal
                    if (m_HangTimer < m_HangTime)
                        m_HangTimer++;	// just inc and hold current m_DecayAve
                    else	// else decay with m_DecayFallAlpha which is RELEASE_TIMECONST
                        m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                     m_DecayFallAlpha * m_Peak;
                }
            }
            else
            {
                // using exponential decay mode
                // perform average of magnitude using 2 averagers each with separate rise and fall time constants
                if (m_Peak > m_AttackAve)	//if magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                    // if magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use m_DecayFallAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                 m_DecayFallAlpha * m_Peak;
            }

            // use greater magnitude of attack or Decay Averager
            if (m_AttackAve > m_DecayAve)
                mag = m_AttackAve;
            else
                mag = m_DecayAve;

            // calc gain depending on which side of knee the magnitude is on
            if (mag <= m_Knee)
                // use fixed gain if below knee
                gain = m_FixedGain;
            else
                // use variable gain if above knee
                gain = AGC_OUTSCALE * powf(10.0, mag * (m_GainSlope - 1.0));

            pOutData[i] = delayedin * gain;
        }
    }
    else
    {
        // manual gain just multiply by m_ManualGain
        for (int i = 0; i < Length; i++)
        {
            pOutData[i] = m_ManualAgcGain * pInData[i];
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Automatic Gain Control calculator for REAL data
//////////////////////////////////////////////////////////////////////
void CAgc::ProcessData(int Length, const float *pInData, float * pOutData)
{
    float       gain;
    float       mag;
    float       delayedin;

    if (m_AgcOn)
    {
        for (int i = 0; i < Length; i++)
        {
            // get latest input sample
            float in = pInData[i];

            // Get delayed sample of input signal
            delayedin = m_SigDelayBuf_r[m_SigDelayPtr];

            // put new input sample into signal delay buffer
            m_SigDelayBuf_r[m_SigDelayPtr++] = in;
            if (m_SigDelayPtr >= m_DelaySamples) //deal with delay buffer wrap around
                m_SigDelayPtr = 0;

            // convert |mag| to log |mag|
            mag = log10f(fabs(in) + MIN_CONSTANT) - LOG_MAX_AMPL;

            // create a sliding window of 'm_WindowSamples' magnitudes and output the peak value within the sliding window
            float tmp = m_MagBuf[m_MagBufPos];   // get oldest mag sample from buffer into tmp
            m_MagBuf[m_MagBufPos++] = mag;       // put latest mag sample in buffer;
            if (m_MagBufPos >= m_WindowSamples)  // deal with magnitude buffer wrap around
                m_MagBufPos = 0;
            if (mag > m_Peak)
            {
                m_Peak = mag;  // if new sample is larger than current peak then use it, no need to look at buffer values
            }
            else
            {
                if (tmp == m_Peak)   // tmp is oldest sample pulled out of buffer
                {                    // if oldest sample pulled out was last peak then need to find next highest peak in buffer
                    m_Peak = -8.0;   // set to lowest value to find next max peak
                    // search all buffer for maximum value and set as new peak
                    for (int i = 0; i < m_WindowSamples; i++)
                    {
                        tmp = m_MagBuf[i];
                        if (tmp > m_Peak)
                            m_Peak = tmp;
                    }
                }
            }

            if (m_UseHang)
            {
                // using hang timer mode
                if (m_Peak > m_AttackAve)
                    // if magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                {
                    // if magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha*m_Peak;
                    m_HangTimer = 0; // reset hang timer
                }
                else
                {
                    // here if decreasing signal
                    if (m_HangTimer < m_HangTime)
                        m_HangTimer++; // just inc and hold current m_DecayAve
                    else
                        // else decay with m_DecayFallAlpha which is RELEASE_TIMECONST
                        m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                     m_DecayFallAlpha * m_Peak;
                }
            }
            else
            {
                // using exponential decay mode
                // perform average of magnitude using 2 averagers each with separate rise and fall time constants
                if (m_Peak > m_AttackAve)
                    // if magnitude is rising (use m_AttackRiseAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackRiseAlpha) * m_AttackAve +
                                  m_AttackRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use  m_AttackFallAlpha time constant)
                    m_AttackAve = (1.0 - m_AttackFallAlpha) * m_AttackAve +
                                  m_AttackFallAlpha * m_Peak;

                if (m_Peak > m_DecayAve)
                    // if magnitude is rising (use m_DecayRiseAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayRiseAlpha) * m_DecayAve +
                                 m_DecayRiseAlpha * m_Peak;
                else
                    // else magnitude is falling (use m_DecayFallAlpha time constant)
                    m_DecayAve = (1.0 - m_DecayFallAlpha) * m_DecayAve +
                                 m_DecayFallAlpha * m_Peak;
            }

            // use greater magnitude of attack or Decay Averager
            if (m_AttackAve > m_DecayAve)
                mag = m_AttackAve;
            else
                mag = m_DecayAve;

            // calc gain depending on which side of knee the magnitude is on
            if (mag <= m_Knee)
                // use fixed gain if below knee
                gain = m_FixedGain;
            else
                // use variable gain if above knee
                gain = AGC_OUTSCALE * powf(10.0, mag * (m_GainSlope - 1.0));
            pOutData[i] = delayedin * gain;
        }
    }
    else
    {	// manual gain just multiply by m_ManualGain
        for (int i = 0; i < Length; i++)
            pOutData[i] = m_ManualAgcGain * pInData[i];
    }

}
