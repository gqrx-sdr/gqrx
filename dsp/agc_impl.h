//////////////////////////////////////////////////////////////////////
// agc_impl.h: interface for the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//      2011-09-24  Adapted for gqrx
//////////////////////////////////////////////////////////////////////
#ifndef AGC_IMPL_H
#define AGC_IMPL_H

//#include "dsp/datatypes.h"
//#include <QMutex>

#define MAX_DELAY_BUF 2048

typedef struct _dCplx
{
    double re;
    double im;
} tDComplex;

#define TYPECPX tDComplex


class CAgc
{
public:
    CAgc();
    virtual ~CAgc();
    void SetParameters(bool AgcOn, bool UseHang, int Threshold, int ManualGain, int Slope, int Decay, double SampleRate);
    void ProcessData(int Length, TYPECPX* pInData, TYPECPX* pOutData);
    void ProcessData(int Length, double* pInData, double* pOutData);

private:
    bool m_AgcOn;				//internal copy of AGC settings parameters
    bool m_UseHang;
    int m_Threshold;
    int m_ManualGain;
    int m_Slope;
    int m_Decay;
    double m_SampleRate;

    double m_SlopeFactor;
    double m_ManualAgcGain;

    double m_DecayAve;
    double m_AttackAve;

    double m_AttackRiseAlpha;
    double m_AttackFallAlpha;
    double m_DecayRiseAlpha;
    double m_DecayFallAlpha;

    double m_FixedGain;
    double m_Knee;
    double m_GainSlope;
    double m_Peak;

    int m_SigDelayPtr;
    int m_MagBufPos;
    int m_DelaySize;
    int m_DelaySamples;
    int m_WindowSamples;
    int m_HangTime;
    int m_HangTimer;

    //QMutex m_Mutex;		//for keeping threads from stomping on each other
    TYPECPX m_SigDelayBuf[MAX_DELAY_BUF];
    double m_MagBuf[MAX_DELAY_BUF];
};

#endif //  AGC_IMPL_H
