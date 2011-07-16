/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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

#include <gr_hier_block2.h>
#include <gr_complex_to_xxx.h>
#include <gr_add_const_ff.h>


class rx_demod_am;


typedef boost::shared_ptr<rx_demod_am> rx_demod_am_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_demod_am.
 *  \param quad_rate The input sample rate.
 *  \param audio_rate The audio rate.
 *  \param dcr Enable DCR
 *
 * This is effectively the public constructor.
 */
rx_demod_am_sptr make_rx_demod_am(float quad_rate, float audio_rate, bool dcr=true);


/*! \brief AM demodulator.
 *  \ingroup DSP
 *
 * This class implements the AM demodulator.
 * AM demodulation is simply a conversion from complex to magnitude.
 * This block does not include any audio filter.
 *
 */
class rx_demod_am : public gr_hier_block2
{

public:
    rx_demod_am(float quad_rate=48000.0, float audio_rate=48000.0, bool dcr=true); // FIXME: could be private
    ~rx_demod_am();

    void set_dcr(bool dcr);
    bool dcr();

private:
    /* GR blocks */
    gr_complex_to_mag_sptr  d_demod;   /*! AM demodulation (complex to magnitude). */
    gr_add_const_ff_sptr    d_dcr;     /*! DC removal (substract 1.0). */


    /* other parameters */
    float  d_quad_rate;     /*! Quadrature rate. */
    float  d_audio_rate;    /*! Audio rate. */
    bool   d_dcr_enabled;   /*! DC removal flag. */

};


#endif // RX_DEMOD_AM_H
