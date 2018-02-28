/*
 * Decimate by power-of-2 classes using half band filters.
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
#ifndef DECIMATOR_H
#define DECIMATOR_H 1

#include <gnuradio/gr_complex.h>

#define MAX_DECIMATION          512
#define MAX_STAGES              9

class Decimator
{
public:
    Decimator();
    virtual    ~Decimator();

    unsigned int    init(unsigned int _decim, unsigned int _att);
    int             process(int samples, gr_complex * pin,
                            gr_complex * pout);

private:

    /**
     * Abstract base class for all the different types of decimate by 2 stages
     */
    class CDec2
    {
    public:
        CDec2() {}
        virtual    ~CDec2(){}
        virtual int DecBy2(int InLength, gr_complex * pInData,
                           gr_complex * pOutData) = 0;
    };

    /**
     * Generic dcimate-by-2 class
     */
    class CHalfBandDecimateBy2 : public CDec2
    {
    public:
        CHalfBandDecimateBy2(int len, const float * pCoef);
        ~CHalfBandDecimateBy2() {
            if(m_pHBFirBuf) delete [] m_pHBFirBuf;
        }
        int     DecBy2(int InLength, gr_complex * pInData,
                       gr_complex * pOutData);

        gr_complex     *m_pHBFirBuf;
        int             m_FirLength;
        const float    *m_pCoef;
    };

    /**
     * 11-tap half-band filter with unrolled loop.
     */
    class CHalfBand11TapDecimateBy2 : public CDec2
    {
    public:
        CHalfBand11TapDecimateBy2(const float * coef);
        ~CHalfBand11TapDecimateBy2() {}
        int     DecBy2(int InLength, gr_complex * pInData,
                       gr_complex * pOutData);

        // coefficients
        float               H0, H2, H4, H5, H6, H8, H10;

        // delay buffer
        std::complex<float> d0, d1, d2, d3, d4, d5, d6, d7, d8, d9;
    };

private:
    int         init_filters_70(unsigned int decimation);
    int         init_filters_100(unsigned int decimation);
    int         init_filters_140(unsigned int decimation);
    void        delete_filters();
    CDec2      *filter_table[MAX_STAGES];

    unsigned int        atten;
    unsigned int        decim;
};

#endif
