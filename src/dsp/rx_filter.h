/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
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

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/fir_filter_ccc.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccc.h>


#define RX_FILTER_MIN_WIDTH 100  /*! Minimum width of filter */

class rx_filter;
class rx_xlating_filter;

typedef boost::shared_ptr<rx_filter> rx_filter_sptr;
typedef boost::shared_ptr<rx_xlating_filter> rx_xlating_filter_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_filter.
 *  \param sample_rate The sample rate.
 *  \param low The lower limit of the bandpass filter.
 *  \param high The upper limit of the filter.
 *  \param trans_width The width of the transition band from
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_filter's constructor is private.
 * make_rxfilter is the public interface for creating new instances.
 */
rx_filter_sptr make_rx_filter(double sample_rate,
                              double low=-5000.0,
                              double high=5000.0,
                              double trans_width=1000.0);

/*! \brief Complex band-pass filter with complex taps.
 *  \ingroup DSP
 *
 * This class encapsulates a complex FIR filter and the code
 * required to generate complex band pass filter taps. It provides a simple
 * interface to set the filter parameters.
 *
 * The user of this class is expected to provide valid parameters and no checks are
 * performed by the accessors (though the taps generator from gr::filter::firdes does perform
 * some sanity checks and throws std::out_of_range in case of bad parameter).
 *
 * \note In order to have proper LSB/USB, we must exchange low and high and reverse their sign
 */
class rx_filter : public gr::hier_block2
{

public:
    rx_filter(double sample_rate=96000.0, double low=-5000.0, double high=5000.0, double trans_width=1000.0); // FIXME: should be private
    ~rx_filter();

    void set_param(double low, double high, double trans_width);

private:
    std::vector<gr_complex> d_taps;
    gr::filter::fir_filter_ccc::sptr  d_bpf;

    double d_sample_rate;
    double d_low;
    double d_high;
    double d_trans_width;
};



/*! \brief Return a shared_ptr to a new instance of rx_xlating_filter.
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
rx_xlating_filter_sptr make_rx_xlating_filter(double sample_rate,
                                              double center=0.0,
                                              double low=-5000.0,
                                              double high=5000.0,
                                              double trans_width=1000.0);


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
 * The user of this class is expected to provide valid parameters and no checks are
 * performed by the accessors (though the taps generator from gr::filter::firdes does perform
 * some sanity checks and throws std::out_of_range in case of bad parameter).
 *
 * \note In order to have proper LSB/USB, we must exchange low and high and reverse their sign?
 */
class rx_xlating_filter : public gr::hier_block2
{

public:
    rx_xlating_filter(double sample_rate=96000.0, double center=0.0, double low=-5000.0, double high=5000.0, double trans_width=1000.0); // FIXME: should be private
    ~rx_xlating_filter();

    void set_offset(double center);
    void set_param(double low, double high, double trans_width);
    void set_param(double center, double low, double high, double trans_width);

private:
    std::vector<gr_complex> d_taps;
    gr::filter::freq_xlating_fir_filter_ccc::sptr d_bpf;

    double d_sample_rate;
    double d_center;
    double d_low;
    double d_high;
    double d_trans_width;
};


#endif // RX_FILTER_H
