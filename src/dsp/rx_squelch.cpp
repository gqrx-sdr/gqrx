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
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <iostream>
#include <QDebug>
#include "dsp/rx_squelch.h"

rx_sql_cc_sptr make_rx_sql_cc(double db, double alpha)
{
    return gnuradio::get_initial_sptr(new rx_sql_cc(db, alpha));
}


rx_sql_cc::rx_sql_cc(double db, double alpha)
    : gr::hier_block2 ("rx_sql_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    d_impl = SQL_SIMPLE;
    sql_pwr = gr::analog::pwr_squelch_cc::make(db, alpha);
    sql_simple = gr::analog::simple_squelch_cc::make(db, alpha);
    connect(self(), 0, sql_simple, 0);
    connect(sql_simple, 0, self(), 0);
}

rx_sql_cc::~rx_sql_cc()
{
}

double rx_sql_cc::threshold()
{
    switch (d_impl)
    {
    case SQL_SIMPLE:
        return sql_simple->threshold();
    case SQL_PWR:
        return sql_pwr->threshold();
    }
    return 0.0;
}

void rx_sql_cc::set_threshold(double db)
{
    sql_simple->set_threshold(db);
    sql_pwr->set_threshold(db);
}

void rx_sql_cc::set_alpha(double alpha)
{
    sql_simple->set_alpha(alpha);
    sql_pwr->set_alpha(alpha);
}

bool rx_sql_cc::unmuted()
{
    switch (d_impl)
    {
    case SQL_SIMPLE:
        return sql_simple->unmuted();
    case SQL_PWR:
        return sql_pwr->unmuted();
    }
    return false;
}

void rx_sql_cc::set_impl(rx_sql_cc::sql_impl_t impl)
{
    if(d_impl == impl)
        return;
    lock();
    switch (d_impl)
    {
    case SQL_SIMPLE:
        disconnect(self(), 0, sql_simple, 0);
        disconnect(sql_simple, 0, self(), 0);
        break;
    case SQL_PWR:
        disconnect(self(), 0, sql_pwr, 0);
        disconnect(sql_pwr, 0, self(), 0);
        break;
    }
    switch (impl)
    {
    case SQL_SIMPLE:
        connect(self(), 0, sql_simple, 0);
        connect(sql_simple, 0, self(), 0);
        break;
    case SQL_PWR:
        connect(self(), 0, sql_pwr, 0);
        connect(sql_pwr, 0, self(), 0);
        break;
    }
    unlock();
    d_impl = impl;
}

rx_sql_cc::sql_impl_t rx_sql_cc::get_impl()
{
    return d_impl;
}

