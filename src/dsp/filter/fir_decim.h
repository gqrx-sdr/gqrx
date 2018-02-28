/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2016 Alexandru Csete OZ9AEC.
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
#pragma once

#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/hier_block2.h>

class fir_decim_cc;

typedef boost::shared_ptr<fir_decim_cc> fir_decim_cc_sptr;
fir_decim_cc_sptr make_fir_decim_cc(unsigned int decim);

class fir_decim_cc : public gr::hier_block2
{
    friend fir_decim_cc_sptr make_fir_decim_cc(unsigned int decim);

//protected:
public:
    fir_decim_cc(unsigned int decim);

public:
    ~fir_decim_cc();

private:
    gr::filter::fir_filter_ccf::sptr        fir1;
    gr::filter::fir_filter_ccf::sptr        fir2;
    gr::filter::fir_filter_ccf::sptr        fir3;
};
