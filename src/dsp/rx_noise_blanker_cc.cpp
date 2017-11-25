/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
 * Copyright 2004-2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY
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
#include <math.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include "dsp/rx_noise_blanker_cc.h"



void rx_nb_cc::forecast (int noutput_items, gr_vector_int &ninput_items_required)
{
    ninput_items_required[0] = d_nb2_bsize;
}


rx_nb_cc_sptr make_rx_nb_cc(double sample_rate, float thld1, float thld2)
{
    return gnuradio::get_initial_sptr(new rx_nb_cc(sample_rate, thld1, thld2));
}


/*! \brief Create noise blanker object.
 *
 * Use make_rx_nb_cc() instead.
 */
rx_nb_cc::rx_nb_cc(double sample_rate, float thld1, float thld2)
    : gr::sync_block ("rx_nb_cc",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(1, 1, sizeof(float))),
      d_nb1_on(false),
      d_nb2_on(false),
      d_sample_rate(sample_rate),
      d_thld_nb1(thld1),
      d_thld_nb2(thld2),
      d_avgmag_nb1(1.0),
      d_avgmag_nb2(1.0),
      d_delidx(2),
      d_sigidx(0),
      d_hangtime(0)
{
    memset(d_delay, 0, 8 * sizeof(gr_complex));


    // Init nb1 params
    d_nb1_dline_size = 2048;
    d_nb1_mask = 2048 - 1;
    d_nb1_n_taps = 64;
    d_nb1_delay = 16;
    d_nb1_two_mu = 1.0e-4;
    d_nb1_gamma = 0.1;
    d_nb1_in_idx = 0;
    d_nb1_lidx = 120.0;
    d_nb1_lidx_min = 120.0;
    d_nb1_lidx_max = 200.0;
    d_nb1_ngamma = 0.001;
    d_nb1_den_mult = 6.25e-10;
    d_nb1_lincr = 1.0;
    d_nb1_ldecr = 3.0;
    memset (d_nb1_d, 0, sizeof(double) * 2048);
    memset (d_nb1_w, 0, sizeof(double) * 2048);

    // Init nb2 params

    d_nb2_bsize = (int)1024;
    d_nb2_fsize = (int)4096;
    d_nb2_ovrlp = 32;
    d_nb2_rate = (int)96000;
    d_nb2_ogain = 0.99;


    // nb2 - initial setup
    int i;
    double Dvals[18] = { 1.0, 2.0, 5.0, 8.0, 10.0, 15.0, 20.0, 30.0, 40.0,
        60.0, 80.0, 120.0, 140.0, 160.0, 180.0, 220.0, 260.0, 300.0 };
    double Mvals[18] = { 0.000, 0.260, 0.480, 0.580, 0.610, 0.668, 0.705, 0.762, 0.800,
        0.841, 0.865, 0.890, 0.900, 0.910, 0.920, 0.930, 0.935, 0.940 };
    double arg, sum, inv_coherent_gain;


    d_nb2_incr = (double)d_nb2_fsize / (double)d_nb2_ovrlp;
    d_nb2_gain = (double)d_nb2_ogain / (double)d_nb2_fsize / (double)d_nb2_ovrlp;
    if (d_nb2_fsize > d_nb2_bsize)
        d_nb2_iasize = d_nb2_fsize;
    else
        d_nb2_iasize = d_nb2_bsize + d_nb2_fsize - d_nb2_incr;
    d_nb2_iainidx = 0;
    d_nb2_iaoutidx = 0;
    if (d_nb2_fsize > d_nb2_bsize)
    {
        if (d_nb2_bsize > d_nb2_incr)  d_nb2_oasize = d_nb2_bsize;
        else                                     d_nb2_oasize = d_nb2_incr;
        d_nb2_oainidx = (d_nb2_fsize - d_nb2_bsize - d_nb2_incr) % d_nb2_oasize;
    }
    else
    {
        d_nb2_oasize = d_nb2_bsize;
        d_nb2_oainidx = d_nb2_fsize - d_nb2_incr;
    }
    d_nb2_init_oainidx = d_nb2_oainidx;
    d_nb2_oaoutidx = 0;
    d_nb2_msize = d_nb2_fsize / 2 + 1;
    d_nb2_window = (double *)malloc(d_nb2_fsize * sizeof(double));
    d_nb2_inaccum = (double *)malloc(d_nb2_iasize * sizeof(double));
    d_nb2_mask = (double *)malloc(d_nb2_msize * sizeof(double));
    d_nb2_save = (double **)malloc(d_nb2_ovrlp * sizeof(double *));
    for (i = 0; i < d_nb2_ovrlp; i++)
        d_nb2_save[i] = (double *)malloc(d_nb2_fsize * sizeof(double));
    d_nb2_outaccum = (double *)malloc(d_nb2_oasize * sizeof(double));
    d_nb2_nsamps = 0;
    d_nb2_saveidx = 0;

    d_nb2_fft1 = new gr::fft::fft_real_fwd(d_nb2_fsize);
    d_nb2_fft2 = new gr::fft::fft_real_rev(d_nb2_fsize);


    arg = 2.0 * 3.1415926 / (double)d_nb2_fsize;
    sum = 0.0;
    for (i = 0; i < d_nb2_fsize; i++)
    {
        d_nb2_window[i] = sqrt (0.54 - 0.46 * cos((double)i * arg));
        sum += d_nb2_window[i];
    }
    inv_coherent_gain = (double)d_nb2_fsize / sum;
    for (i = 0; i < d_nb2_fsize; i++)
        d_nb2_window[i] *= inv_coherent_gain;

    d_nb2_g.msize = d_nb2_msize;
    d_nb2_g.mask = d_nb2_mask;
    d_nb2_g.lambda_y = (double *)malloc(d_nb2_msize * sizeof(double));
    d_nb2_g.lambda_d = (double *)malloc(d_nb2_msize * sizeof(double));
    d_nb2_g.prev_gamma = (double *)malloc(d_nb2_msize * sizeof(double));
    d_nb2_g.prev_mask = (double *)malloc(d_nb2_msize * sizeof(double));


    d_nb2_g.gf1p5 = sqrt(3.1415926) / 2.0;
    {
        double tau = -128.0 / 8000.0 / log(0.98);
        d_nb2_g.alpha = exp(-d_nb2_incr / d_nb2_rate / tau);
    }
    d_nb2_g.eps_floor = 1.0e-300;
    d_nb2_g.gamma_max = 1000.0;
    d_nb2_g.q = 0.2;
    for (i = 0; i < d_nb2_g.msize; i++)
    {
        d_nb2_g.prev_mask[i] = 1.0;
        d_nb2_g.prev_gamma[i] = 1.0;
    }
    d_nb2_g.gmax = 10000.0;

    d_nb2_np.incr = d_nb2_incr;
    d_nb2_np.rate = d_nb2_rate;
    d_nb2_np.msize = d_nb2_msize;
    d_nb2_np.lambda_y = d_nb2_g.lambda_y;
    d_nb2_np.lambda_d = d_nb2_g.lambda_d;

    {
        double tau = -128.0 / 8000.0 / log(0.7);
        d_nb2_np.alphaCsmooth = exp(-d_nb2_np.incr / d_nb2_np.rate / tau);
    }
    {
        double tau = -128.0 / 8000.0 / log(0.96);
        d_nb2_np.alphaMax = exp(-d_nb2_np.incr / d_nb2_np.rate / tau);
    }
    {
        double tau = -128.0 / 8000.0 / log(0.7);
        d_nb2_np.alphaCmin = exp(-d_nb2_np.incr / d_nb2_np.rate / tau);
    }
    {
        double tau = -128.0 / 8000.0 / log(0.3);
        d_nb2_np.alphaMin_max_value = exp(-d_nb2_np.incr / d_nb2_np.rate / tau);
    }
    d_nb2_np.snrq = -d_nb2_np.incr / (0.064 * d_nb2_np.rate);
    {
        double tau = -128.0 / 8000.0 / log(0.8);
        d_nb2_np.betamax = exp(-d_nb2_np.incr / d_nb2_np.rate / tau);
    }
    d_nb2_np.invQeqMax = 0.5;
    d_nb2_np.av = 2.12;
    d_nb2_np.Dtime = 8.0 * 12.0 * 128.0 / 8000.0;
    d_nb2_np.U = 8;
    d_nb2_np.V = (int)(0.5 + (d_nb2_np.Dtime * d_nb2_np.rate / (d_nb2_np.U * d_nb2_np.incr)));
    if (d_nb2_np.V < 4) d_nb2_np.V = 4;
    if ((d_nb2_np.U = (int)(0.5 + (d_nb2_np.Dtime * d_nb2_np.rate / (d_nb2_np.V * d_nb2_np.incr)))) < 1) d_nb2_np.U = 1;
    d_nb2_np.D = d_nb2_np.U * d_nb2_np.V;
    interpM(&d_nb2_np.MofD, d_nb2_np.D, 18, Dvals, Mvals);
    interpM(&d_nb2_np.MofV, d_nb2_np.V, 18, Dvals, Mvals);
    d_nb2_np.invQbar_points[0] = 0.03;
    d_nb2_np.invQbar_points[1] = 0.05;
    d_nb2_np.invQbar_points[2] = 0.06;
    d_nb2_np.invQbar_points[3] = 1.0e300;
    {
        double db;
        db = 10.0 * log10(8.0) / (12.0 * 128 / 8000);
        d_nb2_np.nsmax[0] = pow(10.0, db / 10.0 * d_nb2_np.V * d_nb2_np.incr / d_nb2_np.rate);
        db = 10.0 * log10(4.0) / (12.0 * 128 / 8000);
        d_nb2_np.nsmax[1] = pow(10.0, db / 10.0 * d_nb2_np.V * d_nb2_np.incr / d_nb2_np.rate);
        db = 10.0 * log10(2.0) / (12.0 * 128 / 8000);
        d_nb2_np.nsmax[2] = pow(10.0, db / 10.0 * d_nb2_np.V * d_nb2_np.incr / d_nb2_np.rate);
        db = 10.0 * log10(1.2) / (12.0 * 128 / 8000);
        d_nb2_np.nsmax[3] = pow(10.0, db / 10.0 * d_nb2_np.V * d_nb2_np.incr / d_nb2_np.rate);
    }

    d_nb2_np.p = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.alphaOptHat = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.alphaHat = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.sigma2N = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.pbar = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.p2bar = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.Qeq = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.bmin = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.bmin_sub = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.k_mod = (int *)malloc(d_nb2_np.msize * sizeof(int));
    d_nb2_np.actmin = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.actmin_sub = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.lmin_flag = (int *)malloc(d_nb2_np.msize * sizeof(int));
    d_nb2_np.pmin_u = (double *)malloc(d_nb2_np.msize * sizeof(double));
    d_nb2_np.actminbuff = (double**)malloc(d_nb2_np.U     * sizeof(double*));
    for (i = 0; i < d_nb2_np.U; i++)
        d_nb2_np.actminbuff[i] = (double *)malloc(d_nb2_np.msize * sizeof(double));

    {
        int k, ku;
        d_nb2_np.alphaC = 1.0;
        d_nb2_np.subwc = d_nb2_np.V;
        d_nb2_np.amb_idx = 0;
        for (k = 0; k < d_nb2_np.msize; k++) d_nb2_np.lambda_y[k] = 0.5;
        memcpy(d_nb2_np.p, d_nb2_np.lambda_y, d_nb2_np.msize * sizeof(double));
        memcpy(d_nb2_np.sigma2N, d_nb2_np.lambda_y, d_nb2_np.msize * sizeof(double));
        memcpy(d_nb2_np.pbar, d_nb2_np.lambda_y, d_nb2_np.msize * sizeof(double));
        memcpy(d_nb2_np.pmin_u, d_nb2_np.lambda_y, d_nb2_np.msize * sizeof(double));
        for (k = 0; k < d_nb2_np.msize; k++)
        {
            d_nb2_np.p2bar[k] = d_nb2_np.lambda_y[k] * d_nb2_np.lambda_y[k];
            d_nb2_np.actmin[k] = 1.0e300;
            d_nb2_np.actmin_sub[k] = 1.0e300;
            for (ku = 0; ku < d_nb2_np.U; ku++)
                d_nb2_np.actminbuff[ku][k] = 1.0e300;
        }
        memset(d_nb2_np.lmin_flag, 0, d_nb2_np.msize * sizeof(int));
    }

    d_nb2_nps.incr = d_nb2_incr;
    d_nb2_nps.rate = d_nb2_rate;
    d_nb2_nps.msize = d_nb2_msize;
    d_nb2_nps.lambda_y = d_nb2_g.lambda_y;
    d_nb2_nps.lambda_d = d_nb2_g.lambda_d;

    {
        double tau = -128.0 / 8000.0 / log(0.8);
        d_nb2_nps.alpha_pow = exp(-d_nb2_nps.incr / d_nb2_nps.rate / tau);
    }
    {
        double tau = -128.0 / 8000.0 / log(0.9);
        d_nb2_nps.alpha_Pbar = exp(-d_nb2_nps.incr / d_nb2_nps.rate / tau);
    }
    d_nb2_nps.epsH1 = pow(10.0, 15.0 / 10.0);
    d_nb2_nps.epsH1r = d_nb2_nps.epsH1 / (1.0 + d_nb2_nps.epsH1);

    d_nb2_nps.sigma2N = (double *)malloc(d_nb2_nps.msize * sizeof(double));
    d_nb2_nps.PH1y = (double *)malloc(d_nb2_nps.msize * sizeof(double));
    d_nb2_nps.Pbar = (double *)malloc(d_nb2_nps.msize * sizeof(double));
    d_nb2_nps.EN2y = (double *)malloc(d_nb2_nps.msize * sizeof(double));

    for (i = 0; i < d_nb2_nps.msize; i++)
    {
        d_nb2_nps.sigma2N[i] = 0.5;
        d_nb2_nps.Pbar[i] = 0.5;
    }

    d_nb2_ae.msize = d_nb2_msize;
    d_nb2_ae.lambda_y = d_nb2_g.lambda_y;
    d_nb2_ae.zetaThresh = 0.75;
    d_nb2_ae.psi = 10.0;
    d_nb2_ae.nmask = (double *)malloc(d_nb2_ae.msize * sizeof(double));

    memset (d_nb2_inaccum, 0, d_nb2_iasize * sizeof (double));
    for (i = 0; i < d_nb2_ovrlp; i++)
        memset (d_nb2_save[i], 0, d_nb2_fsize * sizeof (double));
    memset (d_nb2_outaccum, 0, d_nb2_oasize * sizeof (double));
    d_nb2_nsamps   = 0;
    d_nb2_iainidx  = 0;
    d_nb2_iaoutidx = 0;
    d_nb2_oainidx  = d_nb2_init_oainidx;
    d_nb2_oaoutidx = 0;
    d_nb2_saveidx  = 0;
}

rx_nb_cc::~rx_nb_cc()
{
    int i;

    delete d_nb2_fft1;
    delete d_nb2_fft2;
    free(d_nb2_ae.nmask);
    free(d_nb2_nps.EN2y);
    free(d_nb2_nps.Pbar);
    free(d_nb2_nps.PH1y);
    free(d_nb2_nps.sigma2N);

    for (i = 0; i < d_nb2_np.U; i++)
        free(d_nb2_np.actminbuff[i]);
    free(d_nb2_np.actminbuff);
    free(d_nb2_np.pmin_u);
    free(d_nb2_np.lmin_flag);
    free(d_nb2_np.actmin_sub);
    free(d_nb2_np.actmin);
    free(d_nb2_np.k_mod);
    free(d_nb2_np.bmin_sub);
    free(d_nb2_np.bmin);
    free(d_nb2_np.Qeq);
    free(d_nb2_np.p2bar);
    free(d_nb2_np.pbar);
    free(d_nb2_np.sigma2N);
    free(d_nb2_np.alphaHat);
    free(d_nb2_np.alphaOptHat);
    free(d_nb2_np.p);

    free(d_nb2_g.prev_mask);
    free(d_nb2_g.prev_gamma);
    free(d_nb2_g.lambda_d);
    free(d_nb2_g.lambda_y);
    free(d_nb2_outaccum);
    for (i = 0; i < d_nb2_ovrlp; i++)
        free(d_nb2_save[i]);
    free(d_nb2_save);
    free(d_nb2_mask);
    free(d_nb2_inaccum);
    free(d_nb2_window);

}

/*! \brief Receiver noise blanker work method.
 *  \param mooutput_items
 *  \param input_items
 *  \param output_items
 */
int rx_nb_cc::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    const float *in = (const float *) input_items[0];
    float *out = (float *) output_items[0];
    int i;

    boost::mutex::scoped_lock lock(d_mutex);

    // copy data into output buffer then perform the processing on that buffer
    if (noutput_items < d_nb2_bsize)
        return 0;
    else
    {
        for (i = 0; i < d_nb2_bsize; i++)
        {
            out[i] = in[i];
        }

        if (d_nb2_on)
        {
            process_nb2(out, d_nb2_bsize);
        }

        if (d_nb1_on)
        {
            process_nb1(out, d_nb2_bsize);
        }
        return d_nb2_bsize;
    }

    return noutput_items;
}


/*! \brief Perform noise blanker 1 processing.
 *  \param buf The data buffer holding gr_complex samples.
 *  \param num The number of samples in the buffer.
 *
 * Noise blanker 1 is the first noise blanker in the processing chain.
 * It is intended to reduce the effect of impulse type noise.
 *
 * FIXME: Needs different constants for higher sample rates?
 */
void rx_nb_cc::process_nb1(float *buf, int num)
{

    int i, j, idx;
    double c0, c1;
    double y, error, sigma, inv_sigp;
    double nel, nev;
    for (i = 0; i < num; i++)
    {
        d_nb1_d[d_nb1_in_idx] = buf[i];

        y = 0;
        sigma = 0;

        for (j = 0; j < d_nb1_n_taps; j++)
        {
            idx = (d_nb1_in_idx + j + d_nb1_delay) & d_nb1_mask;
            y += d_nb1_w[j] * d_nb1_d[idx];
            sigma += d_nb1_d[idx] * d_nb1_d[idx];
        }
        inv_sigp = 1.0 / (sigma + 1e-10);
        error = d_nb1_d[d_nb1_in_idx] - y;

        buf[i] = y;

        if((nel = error * (1.0 - d_nb1_two_mu * sigma * inv_sigp)) < 0.0) nel = -nel;
        if((nev = d_nb1_d[d_nb1_in_idx] - (1.0 - d_nb1_two_mu * d_nb1_ngamma) * y - d_nb1_two_mu * error * sigma * inv_sigp) < 0.0) 
            nev = -nev;
        if (nev < nel)
        {
            if((d_nb1_lidx += d_nb1_lincr) > d_nb1_lidx_max) d_nb1_lidx = d_nb1_lidx_max;
            else
            if((d_nb1_lidx -= d_nb1_ldecr) < d_nb1_lidx_min) d_nb1_lidx = d_nb1_lidx_min;
        }
        d_nb1_ngamma = d_nb1_gamma * (d_nb1_lidx * d_nb1_lidx) * (d_nb1_lidx * d_nb1_lidx) * d_nb1_den_mult;

        c0 = 1.0 - d_nb1_two_mu * d_nb1_ngamma;
        c1 = d_nb1_two_mu * error * inv_sigp;

        for (j = 0; j < d_nb1_n_taps; j++)
        {
            idx = (d_nb1_in_idx + j + d_nb1_delay) & d_nb1_mask;
            d_nb1_w[j] = c0 * d_nb1_w[j] + c1 * d_nb1_d[idx];
        }
        d_nb1_in_idx = (d_nb1_in_idx + d_nb1_mask) & d_nb1_mask;
    }
}



// Helper functions
double bessI0 (double x)
{
    double res, p;
    if (x == 0.0)
        res = 1.0;
    else
    {
        if (x < 0.0) x = -x;
        if (x <= 3.75)
        {
            p = x / 3.75;
            p = p * p;
            res = ((((( 0.0045813  * p
                      + 0.0360768) * p
                      + 0.2659732) * p
                      + 1.2067492) * p
                      + 3.0899424) * p
                      + 3.5156229) * p
                      + 1.0;
        }
        else
        {
            p = 3.75 / x;
            res = exp (x) / sqrt (x)
                  * (((((((( + 0.00392377  * p
                             - 0.01647633) * p
                             + 0.02635537) * p
                             - 0.02057706) * p
                             + 0.00916281) * p
                             - 0.00157565) * p
                             + 0.00225319) * p
                             + 0.01328592) * p
                             + 0.39894228);
        }
    }
    return res;
}


double bessI1 (double x)
{
    double res, p;
    if (x == 0.0)
        res = 0.0;
    else
    {
        if (x < 0.0) x = -x;
        if (x <= 3.75)
        {
            p = x / 3.75;
            p = p * p;
            res = x 
                  * (((((( 0.00032411  * p
                         + 0.00301532) * p
                         + 0.02658733) * p
                         + 0.15084934) * p
                         + 0.51498869) * p
                         + 0.87890594) * p
                         + 0.5);
        }
        else
        {
            p = 3.75 / x;
            res = exp (x) / sqrt (x)
                  * (((((((( - 0.00420059  * p
                             + 0.01787654) * p
                             - 0.02895312) * p
                             + 0.02282967) * p
                             - 0.01031555) * p
                             + 0.00163801) * p
                             - 0.00362018) * p
                             - 0.03988024) * p
                             + 0.39894228);
        }
    }
    return res;
}


void rx_nb_cc::interpM (double* res, double x, int nvals, double* xvals, double* yvals)
{
    if (x <= xvals[0])
        *res = yvals[0];
    else if (x >= xvals[nvals - 1])
        *res = yvals[nvals - 1];
    else
    {
        int idx = 0;
        double xllow, xlhigh, frac;
        while (x >= xvals[idx])  idx++;
        xllow = log10 (xvals[idx - 1]);
        xlhigh = log10(xvals[idx]);
        frac = (log10 (x) - xllow) / (xlhigh - xllow);
        *res = yvals[idx - 1] + frac * (yvals[idx] - yvals[idx - 1]);
    }
}



/*! \brief Perform noise blanker 2 processing.
 *  \param buf The data buffer holding gr_complex samples.
 *  \param num The number of samples in the buffer.
 *
 * Noise blanker 2 is the second noise blanker in the processing chain.
 * It is intended to reduce non-pulse type noise (i.e. longer time constants).
 *
 * FIXME: Needs different constants for higher sample rates?
 */
void rx_nb_cc::process_nb2(float *buf, int num)
{
        int i, j, k, sbuff, sbegin;
        double g1;
        double f0, f1, f2, f3;
        double sum_prev_p;
        double sum_lambda_y;
        double alphaCtilda;
        double sum_prev_sigma2N;
        double alphaMin, SNR;
        double beta, varHat, invQeq;
        double invQbar;
        double bc;
        double QeqTilda, QeqTildaSub;
        double noise_slope_max;
        int m;
        int N, n;
        double sumPre, sumPost, zeta, zetaT;


        for (i = 0; i < num; i ++)
        {
            d_nb2_inaccum[d_nb2_iainidx] = (double)buf[i];
            d_nb2_iainidx = (d_nb2_iainidx + 1) % d_nb2_iasize;
        }
        d_nb2_nsamps += num;
        while (d_nb2_nsamps >= d_nb2_fsize)
        {
            float *in1 = d_nb2_fft1->get_inbuf();
            for (i = 0, j = d_nb2_iaoutidx; i < d_nb2_fsize; i++, j = (j + 1) % d_nb2_iasize)
                in1[i] = (float)d_nb2_window[i] * (float)d_nb2_inaccum[j];
            d_nb2_iaoutidx = (d_nb2_iaoutidx + d_nb2_incr) % d_nb2_iasize;
            d_nb2_nsamps -= d_nb2_incr;

            d_nb2_fft1->execute();

            // calc_gain
            gr_complex *out1 = d_nb2_fft1->get_outbuf();
            for (k = 0; k < d_nb2_g.msize; k++)
            {
                d_nb2_g.lambda_y[k] = out1[k].real()*out1[k].real();
                d_nb2_g.lambda_y[k] += out1[k].imag()*out1[k].imag();
            }


            // LambdaD
            sum_prev_p = 0.0;
            sum_lambda_y = 0.0;
            sum_prev_sigma2N = 0.0;
            for (k = 0; k < d_nb2_np.msize; k++)
            {
                sum_prev_p += d_nb2_np.p[k];
                sum_lambda_y += d_nb2_np.lambda_y[k];
                sum_prev_sigma2N += d_nb2_np.sigma2N[k];
            }
            for (k = 0; k < d_nb2_np.msize; k++)
            {
                f0 = d_nb2_np.p[k] / d_nb2_np.sigma2N[k] - 1.0;
                d_nb2_np.alphaOptHat[k] = 1.0 / (1.0 + f0 * f0);
            }
            SNR = sum_prev_p / sum_prev_sigma2N;
            alphaMin = std::min(d_nb2_np.alphaMin_max_value, pow (SNR, d_nb2_np.snrq));
            for (k = 0; k < d_nb2_np.msize; k++)
                if (d_nb2_np.alphaOptHat[k] < alphaMin) d_nb2_np.alphaOptHat[k] = alphaMin;
            f1 = sum_prev_p / sum_lambda_y - 1.0;
            alphaCtilda = 1.0 / (1.0 + f1 * f1);
            d_nb2_np.alphaC = d_nb2_np.alphaCsmooth * d_nb2_np.alphaC + (1.0 - d_nb2_np.alphaCsmooth) * std::max (alphaCtilda, d_nb2_np.alphaCmin);
            f2 = d_nb2_np.alphaMax * d_nb2_np.alphaC;
            for (k = 0; k < d_nb2_np.msize; k++)
                d_nb2_np.alphaHat[k] = f2 * d_nb2_np.alphaOptHat[k];
            for (k = 0; k < d_nb2_np.msize; k++)
                d_nb2_np.p[k] = d_nb2_np.alphaHat[k] * d_nb2_np.p[k] + (1.0 - d_nb2_np.alphaHat[k]) * d_nb2_np.lambda_y[k];
            invQbar = 0.0;
            for (k = 0; k < d_nb2_np.msize; k++)
            {
                beta = std::min (d_nb2_np.betamax, d_nb2_np.alphaHat[k] * d_nb2_np.alphaHat[k]);
                d_nb2_np.pbar[k] = beta * d_nb2_np.pbar[k] + (1.0 - beta) * d_nb2_np.p[k];
                d_nb2_np.p2bar[k] = beta * d_nb2_np.p2bar[k] + (1.0 - beta) * d_nb2_np.p[k] * d_nb2_np.p[k];
                varHat = d_nb2_np.p2bar[k] - d_nb2_np.pbar[k] * d_nb2_np.pbar[k];
                invQeq = varHat / (2.0 * d_nb2_np.sigma2N[k] * d_nb2_np.sigma2N[k]);
                if (invQeq > d_nb2_np.invQeqMax) invQeq = d_nb2_np.invQeqMax;
                d_nb2_np.Qeq[k] = 1.0 / invQeq;
                invQbar += invQeq;
            }
            invQbar /= (double)d_nb2_np.msize;
            bc = 1.0 + d_nb2_np.av * sqrt (invQbar);
            for (k = 0; k < d_nb2_np.msize; k++)
            {
                QeqTilda    = (d_nb2_np.Qeq[k] - 2.0 * d_nb2_np.MofD) / (1.0 - d_nb2_np.MofD);
                QeqTildaSub = (d_nb2_np.Qeq[k] - 2.0 * d_nb2_np.MofV) / (1.0 - d_nb2_np.MofV);
                d_nb2_np.bmin[k]     = 1.0 + 2.0 * (d_nb2_np.D - 1.0) / QeqTilda;
                d_nb2_np.bmin_sub[k] = 1.0 + 2.0 * (d_nb2_np.V - 1.0) / QeqTildaSub;
            }
            memset (d_nb2_np.k_mod, 0, d_nb2_np.msize * sizeof (int));
            for (k = 0; k < d_nb2_np.msize; k++)
            {
                f3 = d_nb2_np.p[k] * d_nb2_np.bmin[k] * bc;
                if (f3 < d_nb2_np.actmin[k])
                {
                    d_nb2_np.actmin[k] = f3;
                    d_nb2_np.actmin_sub[k] = d_nb2_np.p[k] * d_nb2_np.bmin_sub[k] * bc;
                    d_nb2_np.k_mod[k] = 1;
                }
            }
            if (d_nb2_np.subwc == d_nb2_np.V)
            {
                if      (invQbar < d_nb2_np.invQbar_points[0]) noise_slope_max = d_nb2_np.nsmax[0];
                else if (invQbar < d_nb2_np.invQbar_points[1]) noise_slope_max = d_nb2_np.nsmax[1];
                else if (invQbar < d_nb2_np.invQbar_points[2]) noise_slope_max = d_nb2_np.nsmax[2];
                else                                        noise_slope_max = d_nb2_np.nsmax[3];
                for (k = 0; k < d_nb2_np.msize; k++)
                {
                    int ku;
                    double min;
                    if (d_nb2_np.k_mod[k])
                        d_nb2_np.lmin_flag[k] = 0;
                    d_nb2_np.actminbuff[d_nb2_np.amb_idx][k] = d_nb2_np.actmin[k];
                    min = 1.0e300;
                    for (ku = 0; ku < d_nb2_np.U; ku++)
                        if (d_nb2_np.actminbuff[ku][k] < min) min = d_nb2_np.actminbuff[ku][k];
                    d_nb2_np.pmin_u[k] = min;
                    if ((d_nb2_np.lmin_flag[k] == 1) 
                        && (d_nb2_np.actmin_sub[k] < noise_slope_max * d_nb2_np.pmin_u[k])
                        && (d_nb2_np.actmin_sub[k] >                   d_nb2_np.pmin_u[k]))
                    {
                        d_nb2_np.pmin_u[k] = d_nb2_np.actmin_sub[k];
                        for (ku = 0; ku < d_nb2_np.U; ku++)
                            d_nb2_np.actminbuff[ku][k] = d_nb2_np.actmin_sub[k];
                    }
                    d_nb2_np.lmin_flag[k] = 0;
                    d_nb2_np.actmin[k] = 1.0e300;
                    d_nb2_np.actmin_sub[k] = 1.0e300;
                }
                if (++d_nb2_np.amb_idx == d_nb2_np.U) d_nb2_np.amb_idx = 0;
                d_nb2_np.subwc = 1;
            }
            else 
            {
                if (d_nb2_np.subwc > 1)
                {
                    for (k = 0; k < d_nb2_np.msize; k++)
                    {
                        if (d_nb2_np.k_mod[k])
                        {
                            d_nb2_np.lmin_flag[k] = 1;
                            d_nb2_np.sigma2N[k] = std::min (d_nb2_np.actmin_sub[k], d_nb2_np.pmin_u[k]);
                            d_nb2_np.pmin_u[k] = d_nb2_np.sigma2N[k];
                        }
                    }
                }
                ++d_nb2_np.subwc;
            }
            memcpy (d_nb2_np.lambda_d, d_nb2_np.sigma2N, d_nb2_np.msize * sizeof (double));


            // gain mode 0 (not 2)
            double gamma, eps_hat, v;
            for (k = 0; k < d_nb2_msize; k++)
            {
                gamma = std::min (d_nb2_g.lambda_y[k] / d_nb2_g.lambda_d[k], d_nb2_g.gamma_max);
                eps_hat = d_nb2_g.alpha * d_nb2_g.prev_mask[k] * d_nb2_g.prev_mask[k] * d_nb2_g.prev_gamma[k]
                    + (1.0 - d_nb2_g.alpha) * std::max (gamma - 1.0, d_nb2_g.eps_floor);
                v = (eps_hat / (1.0 + eps_hat)) * gamma;
                d_nb2_g.mask[k] = d_nb2_g.gf1p5 * sqrt (v) / gamma * exp (- 0.5 * v)
                    * ((1.0 + v) * bessI0 (0.5 * v) + v * bessI1 (0.5 * v));
                {
                    double v2 = std::min (v, 700.0);
                    double eta = d_nb2_g.mask[k] * d_nb2_g.mask[k] * d_nb2_g.lambda_y[k] / d_nb2_g.lambda_d[k];
                    double eps = eta / (1.0 - d_nb2_g.q);
                    double witchHat = (1.0 - d_nb2_g.q) / d_nb2_g.q * exp (v2) / (1.0 + eps);
                    d_nb2_g.mask[k] *= witchHat / (1.0 + witchHat);
                }
                if (d_nb2_g.mask[k] > d_nb2_g.gmax) d_nb2_g.mask[k] = d_nb2_g.gmax;
                if (d_nb2_g.mask[k] != d_nb2_g.mask[k]) d_nb2_g.mask[k] = 0.01;
                d_nb2_g.prev_gamma[k] = gamma;
                d_nb2_g.prev_mask[k] = d_nb2_g.mask[k];
            }


            sumPre = 0.0;
            sumPost = 0.0;
            for (k = 0; k < d_nb2_ae.msize; k++)
            {
                sumPre += d_nb2_ae.lambda_y[k];
                sumPost += d_nb2_mask[k] * d_nb2_mask[k] * d_nb2_ae.lambda_y[k];
            }
            zeta = sumPost / sumPre;
            if (zeta >= d_nb2_ae.zetaThresh)
                zetaT = 1.0;
            else
                zetaT = zeta;
            if (zetaT == 1.0)
                N = 1;
            else
                N = 1 + 2 * (int)(0.5 + d_nb2_ae.psi * (1.0 - zetaT / d_nb2_ae.zetaThresh));
            n = N / 2;
            for (k = n; k < (d_nb2_ae.msize - n); k++)
            {
                d_nb2_ae.nmask[k] = 0.0;
                for (m = k - n; m <= (k + n); m++)
                    d_nb2_ae.nmask[k] += d_nb2_mask[m];
                d_nb2_ae.nmask[k] /= (double)N;
            }
            memcpy (d_nb2_mask + n, d_nb2_ae.nmask, (d_nb2_ae.msize - 2 * n) * sizeof (double));

            // end calc_gain
            gr_complex *in2 = d_nb2_fft2->get_inbuf();
            for (i = 0; i < d_nb2_msize; i++)
            {
                g1 = d_nb2_gain * d_nb2_mask[i];
                in2[i] = gr_complex(g1 * out1[i].real(),g1 * out1[i].imag());
            }

            d_nb2_fft2->execute();

            float *out2 = d_nb2_fft2->get_outbuf();
            for (i = 0; i < d_nb2_fsize; i++)
                d_nb2_save[d_nb2_saveidx][i] = d_nb2_window[i] * out2[i];
            for (i = d_nb2_ovrlp; i > 0; i--)
            {
                sbuff = (d_nb2_saveidx + i) % d_nb2_ovrlp;
                sbegin = d_nb2_incr * (d_nb2_ovrlp - i);
                for (j = sbegin, k = d_nb2_oainidx; j < d_nb2_incr + sbegin; j++, k = (k + 1) % d_nb2_oasize)
                {
                    if ( i == d_nb2_ovrlp)
                        d_nb2_outaccum[k]  = d_nb2_save[sbuff][j];
                    else
                        d_nb2_outaccum[k] += d_nb2_save[sbuff][j];
                }
            }
            d_nb2_saveidx = (d_nb2_saveidx + 1) % d_nb2_ovrlp;
            d_nb2_oainidx = (d_nb2_oainidx + d_nb2_incr) % d_nb2_oasize;
        }
        for (i = 0; i < num; i++)
        {
            buf[i] = (float)d_nb2_outaccum[d_nb2_oaoutidx];
            d_nb2_oaoutidx = (d_nb2_oaoutidx + 1) % d_nb2_oasize;
        }


}

void rx_nb_cc::set_threshold1(float threshold)
{
    if ((threshold >= 1.0) && (threshold <= 20.0))
        d_thld_nb1 = threshold;
}

void rx_nb_cc::set_threshold2(float threshold)
{
    if ((threshold >= 0.0) && (threshold <= 15.0))
        d_thld_nb2 = threshold;
}
