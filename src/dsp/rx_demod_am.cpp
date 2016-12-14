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
#include <gnuradio/filter/firdes.h>


/* Create a new instance of rx_demod_am and return a boost shared_ptr. */
rx_demod_am_sptr make_rx_demod_am(float quad_rate, float audio_rate, bool dcr, bool sync)
{
    return gnuradio::get_initial_sptr(new rx_demod_am(quad_rate, audio_rate, dcr, sync));
}

static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

rx_demod_am::rx_demod_am(float quad_rate, float audio_rate, bool dcr, bool sync)
    : gr::hier_block2 ("rx_demod_am",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_dcr_enabled(dcr),
    d_sync_enabled(sync)
{
    (void) quad_rate;
    (void) audio_rate;

    /* demodulator */
    d_demod = gr::blocks::complex_to_mag::make(1);

    d_taps = gr::filter::firdes::complex_band_pass(1.0, quad_rate, -300, 300, 500);
    d_demod1 = gr::analog::pll_refout_cc::make(0.01, (2*3.1416*1000/quad_rate), (2*3.1416*(-1000)/quad_rate));
    d_demod2 = gr::filter::fir_filter_ccc::make(1, d_taps);
    d_demod3 = gr::blocks::multiply_const_cc::make(gr_complex(0.85,0.85),1);
    d_demod4 = gr::blocks::multiply_conjugate_cc::make(1);
    d_demod5 = gr::blocks::complex_to_real::make(1);


    /* DC removal */
    d_fftaps.resize(2);
    d_fbtaps.resize(2);
    d_fftaps[0] = 1.0;      // FIXME: could be configurable with a specified time constant
    d_fftaps[1] = -1.0;
    d_fbtaps[0] = 0.0;
    d_fbtaps[1] = 0.999;
    d_dcr = gr::filter::iir_filter_ffd::make(d_fftaps, d_fbtaps);

    if (!sync)
    {
        connect(self(), 0, d_demod, 0);
        if (d_dcr_enabled) {
            connect(d_demod, 0, d_dcr, 0);
            connect(d_dcr, 0, self(), 0);
        }
        else {
            connect(d_demod, 0, self(), 0);
        }
    }
    else
    {
        connect(self(), 0, d_demod1, 0);
        connect(d_demod1, 0, d_demod2, 0);
        connect(d_demod2, 0, d_demod3, 0);
        connect(d_demod3, 0, d_demod4, 1);
        connect(self(), 0, d_demod4, 0);
        connect(d_demod4, 0, d_demod5, 0);
        if (d_dcr_enabled) {
            connect(d_demod5, 0, d_dcr, 0);
            connect(d_dcr, 0, self(), 0);
        }
        else {
            connect(d_demod5, 0, self(), 0);
        }

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

    if (!d_sync_enabled)
    {
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
    }
    else
    {
        if (d_dcr_enabled)
        {
            // Switching from ON to OFF
            lock();
            disconnect(d_demod5, 0, d_dcr, 0);
            disconnect(d_dcr, 0, self(), 0);
            connect(d_demod5, 0, self(), 0);
            unlock();
        }
        else
        {
            // Switching from OFF to ON
            lock();
            disconnect(d_demod5, 0, self(), 0);
            connect(d_demod5, 0, d_dcr, 0);
            connect(d_dcr, 0, self(), 0);
            unlock();
        }
    }
    d_dcr_enabled = dcr;
}


/*! \brief Set Sync status.
 *  \param dcr The new status (on or off).
 */
void rx_demod_am::set_sync(bool sync)
{
    if (sync == d_sync_enabled) {
        return;
    }

    if (!d_dcr_enabled)
    {
        if (!d_sync_enabled)
        {
            lock();
            disconnect(d_demod, 0, self(), 0);
            disconnect(self(), 0, d_demod, 0);
            connect(self(), 0, d_demod1, 0);
            connect(d_demod1, 0, d_demod2, 0);
            connect(d_demod2, 0, d_demod3, 0);
            connect(d_demod3, 0, d_demod4, 1);
            connect(self(), 0, d_demod4, 0);
            connect(d_demod4, 0, d_demod5, 0);
            connect(d_demod5, 0, self(), 0);
            unlock();
        }
        else
        {
            lock();
            disconnect(d_demod5, 0, self(), 0);
            disconnect(d_demod4, 0, d_demod5, 0);
            disconnect(d_demod3, 0, d_demod4, 1);
            disconnect(self(), 0, d_demod4, 0);
            disconnect(d_demod2, 0, d_demod3, 0);
            disconnect(d_demod1, 0, d_demod2, 0);
            disconnect(self(), 0, d_demod1, 0);
            connect(self(), 0, d_demod, 0);
            connect(d_demod, 0, self(), 0);
            unlock();
        }
    }
    else
    {
        if (!d_sync_enabled)
        {
            lock();
            disconnect(d_dcr, 0, self(), 0);
            disconnect(d_demod, 0, d_dcr, 0);
            disconnect(self(), 0, d_demod, 0);
            connect(self(), 0, d_demod1, 0);
            connect(d_demod1, 0, d_demod2, 0);
            connect(d_demod2, 0, d_demod3, 0);
            connect(d_demod3, 0, d_demod4, 1);
            connect(self(), 0, d_demod4, 0);
            connect(d_demod4, 0, d_demod5, 0);
            connect(d_demod5, 0, d_dcr, 0);
            connect(d_dcr, 0, self(), 0);
            unlock();
        }
        else
        {
            lock();
            disconnect(d_dcr, 0, self(), 0);
            disconnect(d_demod5, 0, d_dcr, 0);
            disconnect(d_demod4, 0, d_demod5, 0);
            disconnect(d_demod3, 0, d_demod4, 1);
            disconnect(self(), 0, d_demod4, 0);
            disconnect(d_demod2, 0, d_demod3, 0);
            disconnect(d_demod1, 0, d_demod2, 0);
            disconnect(self(), 0, d_demod1, 0);
            connect(self(), 0, d_demod, 0);
            connect(d_demod, 0, d_dcr, 0);
            connect(d_dcr, 0, self(), 0);
            unlock();
        }



    }
    d_sync_enabled = sync;
}



/*! \brief Get current DCR status. */
bool rx_demod_am::dcr()
{
    return d_dcr_enabled;
}

bool rx_demod_am::sync()
{
    return d_sync_enabled;
}
