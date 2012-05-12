/* -*- c++ -*- */
/*
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#include <gr_io_signature.h>
#include "receivers/receiver_base.h"


static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name)
    : gr_hier_block2 (src_name,
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof(float)))
{

}

receiver_base_cf::~receiver_base_cf()
{

}
