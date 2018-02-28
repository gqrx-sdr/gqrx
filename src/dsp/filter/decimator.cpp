/*
 * Decimate by power-of-2 using half band filters.
 *
 * Originally from CuteSdr and modified for nanosdr
 *
 * Copyright 2010 Moe Wheatley.
 * Copyright 2015 Alexandru Csete.
 * All rights reserved.
 *
 * This Software is released under the "Simplified BSD License"
 *
 */
#include <gnuradio/gr_complex.h>
#include <stdio.h>

#include "decimator.h"
#include "filtercoef_hbf_70.h"
#include "filtercoef_hbf_100.h"
#include "filtercoef_hbf_140.h"

#define MAX_HALF_BAND_BUFSIZE 32768
#define DECIM_IS_POWER_OF_2(x)        ((x != 0) && ((x & (~x + 1)) == x))

Decimator::Decimator()
{
    int         i;

    decim = 0;
    atten = 0;

    for (i = 0; i < MAX_STAGES; i++)
        filter_table[i] = 0;
}

Decimator::~Decimator()
{
    delete_filters();
}

unsigned int Decimator::init(unsigned int _decim, unsigned int _att)
{
    if (_decim == decim && _att == atten)
        return decim;

    if (_decim < 2 || _decim > MAX_DECIMATION || !DECIM_IS_POWER_OF_2(_decim))
        return 0;

    delete_filters();

    atten = _att;

    if (atten <= 70)
    {
        decim = init_filters_70(_decim);
    }
    else if (atten <= 100)
    {
        decim = init_filters_100(_decim);
    }
    else
    {
        decim = init_filters_140(_decim);
    }

    return decim;
}

int Decimator::process(int samples, gr_complex * pin, gr_complex * pout)
{
    int         i = 0;
    int         n = samples;

    while (filter_table[i])
        n = filter_table[i++]->DecBy2(n, pin, pin);

    for(i = 0; i < n; i++)
        pout[i] = pin[i];

    return n;
}

void Decimator::delete_filters()
{
    int         i;

    for(i = 0; i < MAX_STAGES; i++)
    {
        if (filter_table[i])
        {
            delete filter_table[i];
            filter_table[i] = 0;
        }
    }
}

int Decimator::init_filters_70(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
        if (decimation >= 4)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_70_11);
            fprintf(stderr, "  DEC %d: HBF_70_11 (unrolled)\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_70_39_LENGTH, HBF_70_39);
            fprintf(stderr, "  DEC %d: HBF_70_39\n", n);
        }

        decimation /= 2;
    }

    return (1 << n);
}

int Decimator::init_filters_100(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
        if (decimation >= 8)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_100_11);
            fprintf(stderr, "  DEC %d: HBF_100_11 (unrolled)\n", n);
        }
        else if (decimation == 4)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_100_19_LENGTH, HBF_100_19);
            fprintf(stderr, "  DEC %d: HBF_100_19\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_100_59_LENGTH, HBF_100_59);
            fprintf(stderr, "  DEC %d: HBF_100_59\n", n);
        }

        decimation /= 2;
    }

    return (1 << n);
}

int Decimator::init_filters_140(unsigned int decimation)
{
    int         n = 0;

    while (decimation >= 2)
    {
        if (decimation >= 16)
        {
            filter_table[n++] = new CHalfBand11TapDecimateBy2(HBF_140_11);
            fprintf(stderr, "  DEC %d: HBF_140_11 (unrolled)\n", n);
        }
        else if (decimation == 8)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_15_LENGTH, HBF_140_15);
            fprintf(stderr, "  DEC %d: HBF_140_15\n", n);
        }
        else if (decimation == 4)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_27_LENGTH, HBF_140_27);
            fprintf(stderr, "  DEC %d: HBF_140_27\n", n);
        }
        else if (decimation == 2)
        {
            filter_table[n++] = new CHalfBandDecimateBy2(HBF_140_87_LENGTH, HBF_140_87);
            fprintf(stderr, "  DEC %d: HBF_140_87\n", n);
        }

        decimation /= 2;
    }

    return (1 << n);
}

Decimator::CHalfBandDecimateBy2::CHalfBandDecimateBy2(int len, const float * pCoef)
    : m_FirLength(len), m_pCoef(pCoef)
{
    gr_complex  CPXZERO(0.0,0.0);

    // create buffer for FIR implementation
    m_pHBFirBuf = new gr_complex[MAX_HALF_BAND_BUFSIZE];
    for (int i = 0; i < MAX_HALF_BAND_BUFSIZE; i++)
        m_pHBFirBuf[i] = CPXZERO;
}

/*
 * Half band filter and decimate by 2 function.
 * Two restrictions on this routine:
 * InLength must be larger or equal to the Number of Halfband Taps
 * InLength must be an even number
 */
int Decimator::CHalfBandDecimateBy2::DecBy2(int InLength, gr_complex * pInData,
                                            gr_complex * pOutData)
{
    gr_complex     acc;
    int     i;
    int     j;
    int     numoutsamples = 0;

    // copy input samples into buffer starting at position m_FirLength-1
    for (i = 0, j = m_FirLength - 1; i < InLength; i++)
        m_pHBFirBuf[j++] = pInData[i];

    // perform decimation FIR filter on even samples
    for (i = 0; i < InLength; i += 2)
    {
        acc = m_pHBFirBuf[i] * m_pCoef[0];
        for (j = 0; j < m_FirLength; j += 2)
            acc += m_pHBFirBuf[i+j] * m_pCoef[j];

        // center coefficient
        acc += m_pHBFirBuf[i+(m_FirLength-1)/2] * m_pCoef[(m_FirLength-1)/2];
        pOutData[numoutsamples++] = acc;
    }

    // need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
    // for FIR wrap around management
    for (i = 0, j = InLength - m_FirLength + 1; i < m_FirLength - 1; i++)
        m_pHBFirBuf[i] = pInData[j++];

    return numoutsamples;
}

/*
 * Decimate by 2 Fixed 11 Tap Halfband filter class implementation
 */
Decimator::CHalfBand11TapDecimateBy2::CHalfBand11TapDecimateBy2(const float * coef)
{
    gr_complex     CPXZERO(0.0, 0.0);

    // preload only the taps that are used since evey other one is zero
    // except center tap 5
    H0 = coef[0];
    H2 = coef[2];
    H4 = coef[4];
    H5 = coef[5];
    H6 = coef[6];
    H8 = coef[8];
    H10 = coef[10];

    d0 = CPXZERO;
    d1 = CPXZERO;
    d2 = CPXZERO;
    d3 = CPXZERO;
    d4 = CPXZERO;
    d5 = CPXZERO;
    d6 = CPXZERO;
    d7 = CPXZERO;
    d8 = CPXZERO;
    d9 = CPXZERO;
}

/*
 * Decimate by 2 Fixed 11 Tap Halfband filter class implementation
 * Two restrictions on this routine:
 * InLength must be larger or equal to the Number of Halfband Taps(11)
 * InLength must be an even number
 * Loop unrolled for speed
 */
int Decimator::CHalfBand11TapDecimateBy2::DecBy2(int InLength,
                                                 gr_complex * pInData,
                                                 gr_complex * pOutData)
{
    // calculate first 10 samples using previous samples in delay buffer
    gr_complex     tmpout[9];

    tmpout[0] = H0 * d0 + H2 * d2 + H4 * d4 + H5 * d5 + H6 * d6 + H8 * d8
            + H10 * pInData[0];

    tmpout[1] = H0 * d2 + H2 * d4 + H4 * d6 + H5 * d7 + H6 * d8
            + H8 * pInData[0] + H10 * pInData[2];

    tmpout[2] = H0 * d4 + H2 * d6 + H4 * d8 + H5 * d9 + H6 * pInData[0]
            + H8 * pInData[2] + H10 * pInData[4];

    tmpout[3] = H0 * d6 + H2 * d8 + H4 * pInData[0] + H5 * pInData[1]
            + H6 * pInData[2] + H8 * pInData[4] + H10 * pInData[6];

    tmpout[4] = H0 * d8 + H2 * pInData[0] + H4 * pInData[2] + H5 * pInData[3]
            + H6 * pInData[4] + H8 * pInData[6] + H10 * pInData[8];

    tmpout[5] = H0 * pInData[0] + H2 * pInData[2] + H4 * pInData[4]
            + H5 * pInData[5] + H6 * pInData[6] + H8 * pInData[8]
            + H10 * pInData[10];

    tmpout[6] = H0 * pInData[2] + H2 * pInData[4] + H4 * pInData[6]
            + H5 * pInData[7] + H6 * pInData[8] + H8 * pInData[10]
            + H10 * pInData[12];

    tmpout[7] = H0 * pInData[4] + H2 * pInData[6] + H4 * pInData[8]
            + H5 * pInData[9] + H6 * pInData[10] + H8 * pInData[12]
            + H10 * pInData[14];

    tmpout[8] = H0 * pInData[6] + H2 * pInData[8] + H4 * pInData[10]
            + H5 * pInData[11] + H6 * pInData[12] + H8 * pInData[14]
            + H10 * pInData[16];

    // now loop through remaining input samples
    gr_complex     *pIn = &pInData[8];
    gr_complex     *pOut = &pOutData[9];
    int         i;

    for(i = 0; i < (InLength - 11 - 6) / 2; i++)
    {
      *pOut++ = (H0 * pIn[0]) + (H2 * pIn[2]) + (H4 * pIn[4]) +
        (H5 * pIn[5]) + (H6 * pIn[6]) + (H8 * pIn[8]) + (H10 * pIn[10]);
      pIn += 2;
    }

    // copy first outputs back into output array so outbuf can be same as inbuf
    pOutData[0] = tmpout[0];
    pOutData[1] = tmpout[1];
    pOutData[2] = tmpout[2];
    pOutData[3] = tmpout[3];
    pOutData[4] = tmpout[4];
    pOutData[5] = tmpout[5];
    pOutData[6] = tmpout[6];
    pOutData[7] = tmpout[7];
    pOutData[8] = tmpout[8];

    // copy last 10 input samples into delay buffer for next time
    pIn = &pInData[InLength-1];
    d9 = *pIn--;
    d8 = *pIn--;
    d7 = *pIn--;
    d6 = *pIn--;
    d5 = *pIn--;
    d4 = *pIn--;
    d3 = *pIn--;
    d2 = *pIn--;
    d1 = *pIn--;
    d0 = *pIn;

    return InLength / 2;
}
