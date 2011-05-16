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
#ifndef RX_FILTER_H
#define RX_FILTER_H

#include <gr_hier_block2.h>


class rx_filter;


typedef boost::shared_ptr<rx_filter> rx_filter_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_filter.
 *  \param sample_rate The sample rate.
 *  \param offset The filter offset.
 *  \param low The lower limit of the bandpass filter.
 *  \param high The upper limit of the filter.
 *  \param trans_width The width of the transition band from
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_filter's constructor is private.
 * make_rxfilter is the public interface for creating new instances.
 */
rx_filter_sptr make_rx_filter(float sample_rate, float offset, float low, float high, float trans_width);


/*! \brief Frequency translating band-pass filter with complex taps.
 *  \ingroup DSP
 *
 * This class encapsulates a frequency translating FIR filter and the code
 * required to generate complex band pass filter taps. It provides a simple
 * interface to set the filter offset and limits and takes care of generating
 * the appropriate taps according to the limits.
 * 
 * The filter limits are relative to the filter offset and thanks to the complex taps
 * they can be both positive and negative.
 *
 */
class rx_filter : public gr_hier_block2
{

public:
    rx_filter(float sample_rate=96000.0, float offset=0.0, float low=-5000.0, float high=5000.0, float trans_width=1000.0); // FIXME: should be private
    ~rx_filter();

    void set_offset(float offset);
    void set_low(float low);
    void set_high(float high);
    void set_trans_width(float trans_width);

    void set_param(float center, float low, float high, float trans_width);
    void set_param(float center, float low, float high);
    void set_param(float low, float high);

private:
    float d_offset;
    float d_low;
    float d_high;
    float d_trans_width;

};



#endif // RX_FILTER_H
