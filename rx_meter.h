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
#ifndef RX_METER_H
#define RX_METER_H

#include <gr_sync_block.h>


class rx_meter_c;

typedef boost::shared_ptr<rx_meter_c> rx_meter_c_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_meter_c.
 *  \param use_avg Whether to use averaging.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_meter_c constructor is private.
 * make_rxfilter is the public interface for creating new instances.
 */
rx_meter_c_sptr make_rx_meter_c(bool use_avg);


/*! \brief Block for measuring signal strength (complex input).
 *  \ingroup DSP
 *
 * This block can be used to meausre the received signal strength.
 * For each group of samples received this block stores the maximum power level,
 * which then can be retrieved using the get_level() and get_level_db()
 * methods.
 */
class rx_meter_c : public gr_sync_block
{
    friend rx_meter_c_sptr make_rx_meter_c(bool use_avg);

private:
    bool   d_use_avg;   /*! Whether to store the average or jsut the latest value. */
    float  d_level;     /*! The current level in the range 0.0 to 1.0 */
    float  d_level_db;  /*! The current level in dBFS with FS = 1.0 */
    float  d_fs;        /*! Full scale value (default = 1.0). */

protected:
    rx_meter_c(bool use_avg);

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
     *  \param use_avg TRUE to enable averaging, FALSE to disable it.
     */
    void set_use_avg(bool use_avg) {d_use_avg = use_avg;}

    /*! \brief Get averaging status
     *  \returns TRUE if averaging is enabled, FALSE if it is disabled.
     */
    bool get_use_avg() {return d_use_avg;}


    /* In case we add a set_fs(), remember that it must be > 0.0 */

    /*! \brief Get full scale value.
     *  \return The current full scale value.
     */
    float get_fs() {return d_fs;}
};


#endif /* RX_METER_H */
