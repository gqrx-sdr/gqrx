/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#ifndef DEFINES_H
#define DEFINES_H

/* Maximum number of receivers */
#define RX_MAX 256


#define TARGET_QUAD_RATE 1e6

/* Number of noice blankers */
#define RECEIVER_NB_COUNT 2

// NB: Remember to adjust filter ranges in MainWindow
#define NB_PREF_QUAD_RATE  96000.f

#define WFM_PREF_QUAD_RATE   240e3 // Nominal channel spacing is 200 kHz

#define RX_FILTER_MIN_WIDTH 100  /*! Minimum width of filter */

#include <memory>
#include <set>
#include <iostream>


#endif // DEFINES_H
