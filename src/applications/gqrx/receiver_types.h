/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef RECEIVER_TYPES_H
#define RECEIVER_TYPES_H

/** Flag used to indicate success or failure of an operation */
enum rx_status {
    STATUS_OK    = 0, /*!< Operation was successful. */
    STATUS_ERROR = 1  /*!< There was an error. */
};

/** Available demodulators. */
enum rx_demod {
    RX_DEMOD_OFF   = 0,  /*!< No receiver. */
    RX_DEMOD_NONE  = 1,  /*!< No demod. Raw I/Q to audio. */
    RX_DEMOD_AM    = 2,  /*!< Amplitude modulation. */
    RX_DEMOD_NFM   = 3,  /*!< Frequency modulation. */
    RX_DEMOD_WFM_M = 4,  /*!< Frequency modulation (wide, mono). */
    RX_DEMOD_WFM_S = 5,  /*!< Frequency modulation (wide, stereo). */
    RX_DEMOD_WFM_S_OIRT = 6,  /*!< Frequency modulation (wide, stereo oirt). */
    RX_DEMOD_SSB   = 7,  /*!< Single Side Band. */
    RX_DEMOD_AMSYNC = 8  /*!< Amplitude modulation (synchronous demod). */
};

/** Supported receiver types. */
enum rx_chain {
    RX_CHAIN_NONE  = 0,   /*!< No receiver, just spectrum analyzer. */
    RX_CHAIN_NBRX  = 1,   /*!< Narrow band receiver (AM, FM, SSB). */
    RX_CHAIN_WFMRX = 2    /*!< Wide band FM receiver (for broadcast). */
};

/** Filter shape (convenience wrappers for "transition width"). */
enum rx_filter_shape {
    FILTER_SHAPE_SOFT = 0,   /*!< Soft: Transition band is TBD of width. */
    FILTER_SHAPE_NORMAL = 1, /*!< Normal: Transition band is TBD of width. */
    FILTER_SHAPE_SHARP = 2   /*!< Sharp: Transition band is TBD of width. */
};

#endif // RECEIVER_TYPES_H
