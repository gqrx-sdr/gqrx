/*
 * Copyright (C) 2015 Analog Devices, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

 /** @file ad9361.h
  * @brief Public interface */

#ifndef __AD9361_H__
#define __AD9361_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup FLAGS MCS Flags
 *
 * @{
 */
/** Flag for ad9361_multichip_sync which verifies interface timing between
    master and all slaves is identical
*/
#define FIXUP_INTERFACE_TIMING	1
/** Flag for ad9361_multichip_sync which checks if master and associated slaves
    have the same sample rate
*/
#define CHECK_SAMPLE_RATES	2

/** @} */

#ifdef _WIN32
#   ifdef LIBAD9361_EXPORTS
#	define __api __declspec(dllexport)
#   else
#	define __api __declspec(dllimport)
#   endif
#elif __GNUC__ >= 4 && !defined(MATLAB_MEX_FILE) && !defined(MATLAB_LOADLIBRARY)
#   define __api __attribute__((visibility ("default")))
#else
#   define __api
#endif

struct iio_context;
struct iio_device;

/**
 * @struct filter_design_parameters
 * @brief Custom Filter Design Parameters
 *
 * A structure for custom filter designs for the AD936X programmable FIR
 * in both TX and RX paths.
 */
struct filter_design_parameters {
    double Rdata; /**< Data rate of digital interface */
    double Fpass; /**< Stop edge frequency in hertz of passband */
    double Fstop; /**< Start edge frequency in hertz of stopband */
    double caldiv; /**< Baseband analog filter calibration divider setting [1-511]  */
    double FIR; /**< Decimation/Interpolation setting of FIR [1,2,4] */
    double HB1; /**< Decimation/Interpolation setting of HB1 [1,2] */
    double DAC_div; /**< Divider enable setting of DAC clock [0,1] */
    const char *Type; /**< Designer mode (only Lowpass supported) */
    const char *RxTx; /**< Filter path [Tx,Rx] */
    double RFbw; /**< 3dB corner of analog filter in hertz */
    double converter_rate; /**< Rate of ADC in hertz */
    double PLL_rate; /**< Rate of PLL in hertz */
    double Fcenter; /**< Center frequency in hertz of bandpass (Unused) */
    double wnom; /**< RF bandwidth of analog filter in hertz */
    double FIRdBmin; /**< Minimum stop band attentuation of the FIR in dB */
    double int_FIR; /**< Enable use of internal FIR filter [0,1] */
    double PLL_mult; /**< Ratio of converter to PLL rate */
    double Apass; /**< Desired passband ripple in dB */
    double Astop; /**< Desired stopband attenuation in dB */
    double phEQ; /**< Enable phase equalization [0,1] */
    double HB2; /**< Decimation/Interpolation setting of HB2 [1,2] */
    double HB3; /**< Decimation/Interpolation setting of HB3 [1,2,3] */
    double maxTaps; /**< Maximum allowed FIR taps */
};

/* ---------------------------------------------------------------------------*/
/* ------------------------- Top-level functions -----------------------------*/
/** @defgroup TopLevel Top-level functions
 * @{ */


/** @brief Multi-chip synchronization (MCS) management
 * @param master A pointer to an iio_device structure
 * @param slaves A double pointer to an iio_device structure
 * @param num_slaves Number of slave devices associated with the master
 * @param flags Control flags for MCS configuration
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_multichip_sync(struct iio_device *master,
		struct iio_device **slaves, unsigned int num_slaves,
		unsigned int flags);

/** @brief FMComms5 specific MCS management
 * @param ctx A pointer to an iio_context structure
 * @param flags Control flags for MCS configuration
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_fmcomms5_multichip_sync(
		struct iio_context *ctx, unsigned int flags);

/** @brief Baseband rate configuration with generic filter support
 * @param dev A pointer to an iio_device structure
 * @param rate Rate in samples per second of desired baseband rate
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> Three possible filters are loaded based on required rate and
 * associated decimation/interpolation. These filters are generally very wide
 * band and not waveform specific. */
__api int ad9361_set_bb_rate(struct iio_device *dev, unsigned long rate);

/** @brief Enable or disable transmit and receiver FIRs simultaneously
 * @param dev A pointer to an iio_device structure
 * @param enable Integer to enable FIRs when 1 or disable when 0
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_set_trx_fir_enable(struct iio_device *dev, int enable);

/** @brief Get current enable value of transmit and receiver FIRs
 * @param dev A pointer to an iio_device structure
 * @param enable Returned integer value of FIR enabled
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_get_trx_fir_enable(struct iio_device *dev, int *enable);

/** @brief Design custom FIR filter from specific design criteria
 * @param parameters A pointer filter designer structure
 * @param taps A pointer to taps of designed filter
 * @param num_taps A pointer to integer number of taps designed in taps
 * @param gain Integer gain for designed filter
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_generate_fir_taps(struct filter_design_parameters *parameters,
                                   short *taps, int *num_taps, int *gain);

/** @brief Calculate the clock path rates for both transmit and receiver paths
 * @param tx_sample_rate Sample rate in samples per second of desired baseband rate
 * @param rate_gov Rate governor enable setting forcing HB3=3 when enabled
 * @param rx_path_clks A pointer to a unsigned long variable where the 6 RX path rates should be stored
 * @param tx_path_clks A pointer to a unsigned long variable where the 6 TX path rates should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_calculate_rf_clock_chain(unsigned long tx_sample_rate,
                                          unsigned long rate_gov,
                                          unsigned long *rx_path_clks,
                                          unsigned long *tx_path_clks);

/** @brief Calculate the clock path rates and default filter settings for both transmit and receiver for a desired baseband rate
 * @param fdpTX Filter design parameters structure where TX filter design parameters will be stored
 * @param fdpRX Filter design parameters structure where RX filter design parameters will be stored
 * @param sample_rate Desired basedband sample rate in samples per second for both RX and TX filter configurations
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_calculate_rf_clock_chain_fdp(struct filter_design_parameters *fdpTX,
                                              struct filter_design_parameters *fdpRX,
                                              unsigned long sample_rate);

/** @brief Baseband rate configuration with custom filter support based on desired baseband sample rate
 * @param dev A pointer to an iio_device structure
 * @param rate Rate in samples per second of desired baseband rate
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> Designed filter will have the following configuration:
 * Fpass = rate / 3
 * Fstop = Fpass * 1.25
 * wnomTX = 1.6 * Fstop
 * wnomRX = 1.4 * Fstop */
__api int ad9361_set_bb_rate_custom_filter_auto(struct iio_device *dev,
                                                unsigned long rate);

/** @brief Baseband rate configuration with custom filter support based on desired baseband sample rate and simplified filter configuration
 * @param dev A pointer to an iio_device structure
 * @param rate Rate in samples per second of desired baseband rate
 * @param Fpass Stop edge frequency in hertz of passband
 * @param Fstop Start edge frequency in hertz of stopband
 * @param wnom_tx TX RF bandwidth of analog filter in hertz
 * @param wnom_rx RX RF bandwidth of analog filter in hertz
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api int ad9361_set_bb_rate_custom_filter_manual(struct iio_device *dev,
                                                  unsigned long rate, unsigned long Fpass,
                                                  unsigned long Fstop, unsigned long wnom_tx,
                                                  unsigned long wnom_rx);

/** @brief FMComms5 phase synchronize all TX and RX channels together
 * @param ctx A pointer to an iio_context structure
 * @param lo Frequency in hertz of LO for TX and RX
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned. If -2 is returned calibration failed
 *
 * <b>NOTES:</b> To perform calibration the following side effects occur:
 * - RF bandwidths of both TX and RX are expanded to the current sample rate. It can be changed after calibration without effecting phase synchronization.
 * - DDSs are enabled and left on after synchronization. Changing these DDSs or switching to DMA sources will not effect phase synchronization.
 * - TX and RX LOs are set to the same frequency based on the input provided. LO changes can invalidate phase synchronization.
 * - AGCs are set to manual mode at predetermined hardware gains for TX and RX. Gain changes can invalidate phase synchronization.
 *
 * Phase synchronization is valid until the LOs are retuned or sample rates change or gains are modified.
 *
 * <b>External Links:</b>
 * - <a href="https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms5-ebz/multi-chip-sync">Detailed information on synchronization process</a>
 * - <a href="https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms5-ebz/hardware">Phase synchronization performance can depend on revision of hardware</a>*/
__api int ad9361_fmcomms5_phase_sync(struct iio_context *ctx, long long lo);

/** @} */

#ifdef __cplusplus
}
#endif

#undef __api

#endif /* __AD9361_H__ */
