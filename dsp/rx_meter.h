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
#ifndef RX_METER_H
#define RX_METER_H

#include <gnuradio/sync_block.h>

enum detector_type_e {
    DETECTOR_TYPE_NONE   = 0,
    DETECTOR_TYPE_SAMPLE = 1,
    DETECTOR_TYPE_MIN    = 2,
    DETECTOR_TYPE_MAX    = 3,
    DETECTOR_TYPE_AVG    = 4,
    DETECTOR_TYPE_RMS    = 5
};


class rx_meter_c;

typedef boost::shared_ptr<rx_meter_c> rx_meter_c_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_meter_c.
 *  \param detector Detector type.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_meter_c constructor is private.
 * make_rxfilter is the public interface for creating new instances.
 */
rx_meter_c_sptr make_rx_meter_c(int detector=DETECTOR_TYPE_RMS);


/*! \brief Block for measuring signal strength (complex input).
 *  \ingroup DSP
 *
 * This block can be used to meausre the received signal strength.
 * For each group of samples received this block stores the maximum power level,
 * which then can be retrieved using the get_level() and get_level_db()
 * methods.
 */
class rx_meter_c : public gr::sync_block
{
    friend rx_meter_c_sptr make_rx_meter_c(int detector);

protected:
    rx_meter_c(int detector=DETECTOR_TYPE_RMS);

public:
    ~rx_meter_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    /*! \brief Get the current signal level. */
    float get_level();

    /*! \brief Get the current signal level in dBFS. */
    float get_level_db();

    /*! \brief Enable or disable averaging.
     *  \param detector Detector type.
     */
    void set_detector_type(int detector);

    /*! \brief Get averaging status
     *  \returns TRUE if averaging is enabled, FALSE if it is disabled.
     */
    int get_detector_type() {return d_detector;}

private:
    int    d_detector;  /*! Detector type. */
    float  d_level;     /*! The current level in the range 0.0 to 1.0 */
    float  d_level_db;  /*! The current level in dBFS with FS = 1.0 */
    float  d_sum;       /*! Sum of msamples. */
    float  d_sumsq;     /*! Sum of samples squared. */
    int    d_num;       /*! Number of samples in d_sum and d_sumsq. */

    void reset_stats();
};


#endif /* RX_METER_H */
