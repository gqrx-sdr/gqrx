/* -*- c++ -*- */
/*
 * Copyright 2015 Alexandru Csete.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_MYMOD_HBF_DECIM_H
#define INCLUDED_MYMOD_HBF_DECIM_H

#include <gnuradio/sync_decimator.h>
#include <gnuradio/types.h>
#include "filter/decimator.h"

class hbf_decim;
typedef boost::shared_ptr<hbf_decim> hbf_decim_sptr;
hbf_decim_sptr make_hbf_decim(unsigned int decim);

/**
 * Decimator block using half-band filters.
 */
class hbf_decim : virtual public gr::sync_decimator
{
    friend hbf_decim_sptr make_hbf_decim(unsigned int decim);

protected:
    hbf_decim(unsigned int decim);

public:
    ~hbf_decim();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

private:
    Decimator          *dec;
    unsigned int        decimation;
};


#endif /* INCLUDED_MYMOD_HBF_DECIM_H */

