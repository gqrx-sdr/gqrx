/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
 * Copyright 2013 Vesa Solonen OH2JCP.
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
#include <dsp/rx_demod_am.h>


/* Create a new instance of rx_demod_am and return a boost shared_ptr. */
rx_demod_am_sptr make_rx_demod_am(float quad_rate, bool dcr)
{
    return gnuradio::get_initial_sptr(new rx_demod_am(quad_rate, dcr));
}

static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

rx_demod_am::rx_demod_am(float quad_rate, bool dcr)
    : gr::hier_block2 ("rx_demod_am",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_dcr_enabled(dcr)
{
    (void) quad_rate;

    /* demodulator */
    d_demod = gr::blocks::complex_to_mag::make(1);

    /* connect blocks */
    connect(self(), 0, d_demod, 0);

    /* DC removal */
    d_fftaps.resize(2);
    d_fbtaps.resize(2);
    d_fftaps[0] = 1.0;      // FIXME: could be configurable with a specified time constant
    d_fftaps[1] = -1.0;
    d_fbtaps[0] = 0.0;
    d_fbtaps[1] = 0.999;
    d_dcr = gr::filter::iir_filter_ffd::make(d_fftaps, d_fbtaps);

    if (d_dcr_enabled) {
        connect(d_demod, 0, d_dcr, 0);
        connect(d_dcr, 0, self(), 0);
    }
    else {
        connect(d_demod, 0, self(), 0);
    }

}

rx_demod_am::~rx_demod_am ()
{

}

/*! \brief Set DCR status.
 *  \param dcr The new status (on or off).
 */
void rx_demod_am::set_dcr(bool dcr)
{
    if (dcr == d_dcr_enabled) {
        return;
    }

    if (d_dcr_enabled)
    {
        // Switching from ON to OFF
        lock();
        disconnect(d_demod, 0, d_dcr, 0);
        disconnect(d_dcr, 0, self(), 0);
        connect(d_demod, 0, self(), 0);
        unlock();
    }
    else
    {
        // Switching from OFF to ON
        lock();
        disconnect(d_demod, 0, self(), 0);
        connect(d_demod, 0, d_dcr, 0);
        connect(d_dcr, 0, self(), 0);
        unlock();
    }

    d_dcr_enabled = dcr;
}

/*! \brief Get current DCR status. */
bool rx_demod_am::dcr()
{
    return d_dcr_enabled;
}
