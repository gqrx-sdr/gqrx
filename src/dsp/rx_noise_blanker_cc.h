/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#ifndef RX_NB_CC_H
#define RX_NB_CC_H

#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/fft/fft.h>
#include <boost/thread/mutex.hpp>

class rx_nb_cc;

typedef boost::shared_ptr<rx_nb_cc> rx_nb_cc_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_nb_cc.
 *  \param sample_rate The samle rate (default = 96000).
 *  \param threshold Noise blanker threshold. Range 0.0 to 1.0 (TBC)
 *
 * This is effectively the public constructor for a new noise blanker block.
 * To avoid accidental use of raw pointers, the rx_nb_cc constructor is private.
 * make_rx_nb_cc is the public interface for creating new instances.
 */
rx_nb_cc_sptr make_rx_nb_cc(double sample_rate=96000.0, float thld1=3.3, float thld2=2.5);


/*! \brief Noise blanker block.
 *  \ingroup DSP
 *
 * This block implements noise blanking filters based on the noise blanker code
 * from DTTSP.

 *
 */
class rx_nb_cc : public gr::sync_block
{
    friend rx_nb_cc_sptr make_rx_nb_cc(double sample_rate, float thld1, float thld2);

protected:
    rx_nb_cc(double sample_rate, float thld1, float thld2);

public:
    ~rx_nb_cc();
    void forecast (int noutput_items, gr_vector_int &ninput_items_required);

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void set_sample_rate(double sample_rate) { d_sample_rate = sample_rate; }
    void set_nb1_on(bool nb1_on) 
    {
        memset (d_nb1_d, 0, sizeof(double) * 2048);
        memset (d_nb1_w, 0, sizeof(double) * 2048);
        d_nb1_in_idx = 0;
        d_nb1_on = nb1_on; 
    }
    void set_nb2_on(bool nb2_on) 
    {
        memset (d_nb2_inaccum, 0, d_nb2_iasize * sizeof (double));
        for (int i = 0; i < d_nb2_ovrlp; i++)
            memset (d_nb2_save[i], 0, d_nb2_fsize * sizeof (double));
        memset (d_nb2_outaccum, 0, d_nb2_oasize * sizeof (double));
        d_nb2_nsamps   = 0;
        d_nb2_iainidx  = 0;
        d_nb2_iaoutidx = 0;
        d_nb2_oainidx  = d_nb2_init_oainidx;
        d_nb2_oaoutidx = 0;
        d_nb2_saveidx  = 0;
        d_nb2_on = nb2_on; 
    }

    bool get_nb1_on() { return d_nb1_on; }
    bool get_nb2_on() { return d_nb2_on; }
    void set_threshold1(float threshold);
    void set_threshold2(float threshold);

private:
    void process_nb1(gr_complex *buf, int num);
    void process_nb2(gr_complex *buf, int num);
    void interpM (double* res, double x, int nvals, double* xvals, double* yvals);



private:
    boost::mutex  d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    bool   d_nb1_on;        /*! Current NB1 status (true/false). */
    bool   d_nb2_on;        /*! Current NB2 status (true/false). */
    double d_sample_rate;   /*! Current sample rate. */
    float  d_thld_nb1;      /*! Current threshold for noise blanker 1 (1.0 to 20.0 TBC). */
    float  d_thld_nb2;      /*! Current threshold for noise blanker 2 (0.0 to 15.0 TBC). */
    float  d_avgmag_nb1;    /*! Average magnitude. */
    float  d_avgmag_nb2;    /*! Average magnitude. */
    gr_complex d_avgsig, d_delay[8];
    int    d_delidx, d_sigidx, d_hangtime;   // FIXME: need longer buffer for higher sampel rates?
    int    d_nb1_state;
    int    d_nb2_state;


    // NB1 parameters
    int d_nb1_dline_size;
    int d_nb1_mask;
    int d_nb1_n_taps;
    int d_nb1_delay;
    double d_nb1_two_mu;
    double ed_nb1_gamma;
    int d_nb1_in_idx;
    double d_nb1_lidx;
    double d_nb1_lidx_min;
    double d_nb1_lidx_max;
    double d_nb1_ngamma;
    double d_nb1_gamma;
    double d_nb1_den_mult;
    double d_nb1_lincr;
    double d_nb1_ldecr;
    double d_nb1_d[2048];
    double d_nb1_w[2048];

    // NB2 parameters
    int d_nb2_fsize;
    int d_nb2_bsize;
    int d_nb2_ovrlp;
    int d_nb2_incr;
    double* d_nb2_window;
    int d_nb2_iasize;
    double* d_nb2_inaccum;
    double* d_nb2_forfftin;
    gr_complex* d_nb2_forfftout;
    int d_nb2_msize;
    double* d_nb2_mask;
    gr_complex* d_nb2_revfftin;
    double* d_nb2_revfftout;
    double** d_nb2_save;
    int d_nb2_oasize;
    double* d_nb2_outaccum;
    double d_nb2_rate;
    int d_nb2_wintype;
    double d_nb2_ogain;
    double d_nb2_gain;
    int d_nb2_nsamps;
    int d_nb2_iainidx;
    int d_nb2_iaoutidx;
    int d_nb2_init_oainidx;
    int d_nb2_oainidx;
    int d_nb2_oaoutidx;
    int d_nb2_saveidx;
    //fftw_plan Rfor;
    //fftw_plan Rrev;
    struct _g
    {
        double msize;
        double* mask;
        double* y;
        double* lambda_y;
        double* lambda_d;
        double* prev_mask;
        double* prev_gamma;
        double gf1p5;
        double alpha;
        double eps_floor;
        double gamma_max;
        double q;
        double gmax;
    } d_nb2_g;
    struct _npest
    {
        int incr;
        double rate;
        int msize;
        double* lambda_y;
        double* lambda_d;
        double* p;
        double* alphaOptHat;
        double alphaC;
        double alphaCsmooth;
        double alphaCmin;
        double* alphaHat;
        double alphaMax;
        double* sigma2N;
        double alphaMin_max_value;
        double snrq;
        double betamax;
        double* pbar;
        double* p2bar;
        double invQeqMax;
        double av;
        double* Qeq;
        int U;
        double Dtime;
        int V;
        int D;
        double MofD;
        double MofV;
        double* bmin;
        double* bmin_sub;
        int* k_mod;
        double* actmin;
        double* actmin_sub;
        int subwc;
        int* lmin_flag;
        double* pmin_u;
        double invQbar_points[4];
        double nsmax[4];
        double** actminbuff;
        int amb_idx;
    } d_nb2_np;
    struct _npests
    {
        int incr;
        double rate;
        int msize;
        double* lambda_y;
        double* lambda_d;
        
        double alpha_pow;
        double alpha_Pbar;
        double epsH1;
        double epsH1r;

        double* sigma2N;
        double* PH1y;
        double* Pbar;
        double* EN2y;
    } d_nb2_nps;
    struct _ae
    {
        int msize;
        double* lambda_y;
        double zetaThresh;
        double psi;
        double* nmask;
    } d_nb2_ae;
    gr::fft::fft_real_fwd  *d_nb2_fft1;
    gr::fft::fft_real_rev  *d_nb2_fft2;

};



#endif /* RX_NB_CC_H */
