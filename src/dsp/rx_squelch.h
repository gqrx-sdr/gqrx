/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#ifndef RX_SQUELCH_CC_H
#define RX_SQUELCH_CC_H

#include <gnuradio/gr_complex.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/pwr_squelch_cc.h>
#include <gnuradio/analog/simple_squelch_cc.h>


class rx_sql_cc;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_sql_cc> rx_sql_cc_sptr;
#else
typedef std::shared_ptr<rx_sql_cc> rx_sql_cc_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of rx_sql_cc.
 *  \param db threshold (in dB) for squelch
 *  \param alpha Gain of averaging filter. Defaults to 0.0001.
 */
rx_sql_cc_sptr make_rx_sql_cc(double db, double alpha = 0.0001);

/*! \brief Squelch implementation switching block.
 *  \ingroup DSP
 *
 * This block allows to select between simple_squelch and pwr_squelch blocks
 */
class rx_sql_cc : public gr::hier_block2
{
    friend rx_sql_cc_sptr make_rx_sql_cc(double db, double alpha);

protected:
    rx_sql_cc(double db, double alpha);

public:
    typedef enum{
        SQL_SIMPLE = 0,
        SQL_PWR
    } sql_impl_t;

    ~rx_sql_cc();
    double threshold();
    void set_threshold(double db);
    void set_alpha(double alpha);
    bool unmuted();
    void set_impl(sql_impl_t impl);
    sql_impl_t get_impl();

private:
    gr::analog::pwr_squelch_cc::sptr sql_pwr;        /*!< Pwr Squelch (squelch-triggered recording mode). */
    gr::analog::simple_squelch_cc::sptr sql_simple;  /*!< Simple Squelch (generic mode). */
    sql_impl_t d_impl;
};

#endif /* RX_SQUELCH_CC_H */
