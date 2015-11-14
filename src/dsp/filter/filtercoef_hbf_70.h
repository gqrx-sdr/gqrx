/*
 * Half-band filter coefficients with 70 dB stop band attenuation.
 *
 * Copyright 2015 Alexandru Csete
 * All rights reserved.
 *
 * This Software is released under the Simplified BSD License.
 *
 */
#ifndef FILTERCOEF_HBF_70_H
#define FILTERCOEF_HBF_70_H


// Normalized 70 dB alias free bandwidths
#define HBF_70_11_BW     0.2f
#define HBF_70_39_BW     0.4f

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 11
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Alias free BW     : 0.2
 * Passband ripple   : +/- 3.2e-3 dB
 * Stop band         : -70 dB
 */
#define HBF_70_11_LENGTH     11
const float HBF_70_11[HBF_70_11_LENGTH] =
{
	0.009707733567516,
	0.0,
	-0.05811715559409,
	0.0,
	0.2985919803575,
	0.5,
	0.2985919803575,
	0.0,
	-0.05811715559409,
	0.0,
	0.009707733567516
};

/*
 * Discrete-Time FIR Filter (real)
 * -------------------------------
 * Filter Structure  : Direct-Form FIR
 * Filter Length     : 39
 * Stable            : Yes
 * Linear Phase      : Yes (Type 1)
 * Alias free BW     : 0.4
 * Passband ripple   : +/- 3e-3 dB
 * Stop band         : -70 dB
 */
#define HBF_70_39_LENGTH     39
const float HBF_70_39[HBF_70_39_LENGTH] =
{
    -0.0006388614035059,
    0.0,
    0.001631195589637,
    0.0,
    -0.003550156604839,
    0.0,
    0.006773869396241,
    0.0,
    -0.01188293607946,
    0.0,
    0.01978182909123,
    0.0,
    -0.03220528568021,
    0.0,
    0.05351179043142,
    0.0,
    -0.09972534459278,
    0.0,
    0.3161340967929,
    0.5,
    0.3161340967929,
    0.0,
    -0.09972534459278,
    0.0,
    0.05351179043142,
    0.0,
    -0.03220528568021,
    0.0,
    0.01978182909123,
    0.0,
    -0.01188293607946,
    0.0,
    0.006773869396241,
    0.0,
    -0.003550156604839,
    0.0,
    0.001631195589637,
    0.0,
    -0.0006388614035059
};

#endif
