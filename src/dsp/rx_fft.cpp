/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include "dsp/rx_fft.h"
/**   rx_fft_f     **/

rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, double audio_rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft<float> ("rx_fft_f", fftsize, audio_rate, wintype));
}

rx_fft_c_sptr make_rx_fft_c (unsigned int fftsize, double rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft<gr_complex> ("rx_fft_c", fftsize, rate, wintype));
}
