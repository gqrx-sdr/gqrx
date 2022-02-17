//////////////////////////////////////////////////////////////////////
// agc_impl.h: interface for the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//  2010-09-15  Initial creation MSW
//  2011-03-27  Initial release
//  2011-09-24  Adapted for gqrx
//  2022-01-10  Rewritten with low complexity alogo
//////////////////////////////////////////////////////////////////////
#ifndef AGC_IMPL_H
#define AGC_IMPL_H

#include <complex>
#include <vector>

#define TYPECPX std::complex<float>
#define TYPEFLOAT float

class CAgc
{
public:
    CAgc();
    virtual ~CAgc();
    void SetParameters(double sample_rate, bool agc_on, int target_level,
                       int manual_gain, int max_gain, int attack, int decay,
                       int hang, bool force = false);
    void ProcessData(TYPECPX * pOutData, const TYPECPX * pInData, int Length);

private:
    float get_peak();
    void update_buffer(int p);

    float d_sample_rate;
    bool d_agc_on;
    int d_target_level;
    int d_manual_gain;
    int d_max_gain;
    int d_attack;
    int d_decay;
    int d_hang;

    TYPEFLOAT d_target_mag;
    int d_hang_samp;
    int d_buf_samples;
    int d_buf_size;
    int d_max_idx;
    int d_buf_p;
    int d_hang_counter;
    TYPEFLOAT d_max_gain_mag;
    TYPEFLOAT d_current_gain;
    TYPEFLOAT d_target_gain;
    TYPEFLOAT d_decay_step;
    TYPEFLOAT d_attack_step;
    TYPEFLOAT d_floor;

    std::vector<TYPECPX> d_sample_buf;
    std::vector<float>   d_mag_buf;

    TYPEFLOAT d_prev_dbg;
};

#endif //  AGC_IMPL_H
