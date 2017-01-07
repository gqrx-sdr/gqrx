/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
 * Copyright 2013 Vesa Solonen OH2JCP.
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
#ifndef RX_DEMOD_AM_H
#define RX_DEMOD_AM_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/complex_to_mag.h>
#include <gnuradio/filter/iir_filter_ffd.h>
#include <vector>


class rx_demod_am;

typedef boost::shared_ptr<rx_demod_am> rx_demod_am_sptr;

/*! \brief Return a shared_ptr to a new instance of rx_demod_am.
 *  \param quad_rate The input sample rate.
 *  \param dcr Enable DCR
 *
 * This is effectively the public constructor.
 */
rx_demod_am_sptr make_rx_demod_am(float quad_rate, bool dcr=true);

/*! \brief AM demodulator.
 *  \ingroup DSP
 *
 * This class implements the AM demodulator as envelope detector.
 * AM demodulation is simply a conversion from complex to magnitude.
 * This block implements an optional IIR DC-removal filter for the demodulated signal.
 *
 */
class rx_demod_am : public gr::hier_block2
{

public:
    rx_demod_am(float quad_rate, bool dcr=true); // FIXME: could be private
    ~rx_demod_am();

    void set_dcr(bool dcr);
    bool dcr();

private:
    /* GR blocks */
    gr::blocks::complex_to_mag::sptr    d_demod;  /*! AM demodulation (complex to magnitude). */
    gr::filter::iir_filter_ffd::sptr    d_dcr;    /*! DC removal (IIR high pass). */

    /* other parameters */
    bool   d_dcr_enabled;   /*! DC removal flag. */

    /* IIR DC-removal filter taps */
    std::vector<double> d_fftaps;   /*! Feed forward taps. */
    std::vector<double> d_fbtaps;   /*! Feed back taps. */

};


#endif // RX_DEMOD_AM_H
