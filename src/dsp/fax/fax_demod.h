/*
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * fax demodulator hier block header
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

#ifndef FAX_DEMOD_H
#define FAX_DEMOD_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/firdes.h>
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#else
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#endif
#include <gnuradio/analog/quadrature_demod_cf.h>
#include "fax_demod.h"
#include "dsp/fax/fax_decoder.h"
#include "dsp/fax/fax_store.h"
#include <vector>


namespace gr {
   namespace fax {
      class fax_demod : public gr::hier_block2
      {

         public:
#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<fax_demod> sptr;
#else
            typedef std::shared_ptr<fax_demod> sptr;
#endif

            static sptr make(float quad_rate, float black_freq, float white_freq,float lpm,float ioc);

            void    set_quad_rate(float quad_rate);
            float   quad_rate() const;
            void    set_black_freq(float black_freq);
            float   black_freq() const;
            void    set_white_freq(float white_freq);
            float   white_freq() const;
            void    set_lpm(float lpm);
            float   lpm() const;
            void    set_ioc(float ioc);
            float   ioc() const;
            void    set_decoder_state(int state);
            int     decoder_state() const;

            void reset();

            int get_data(unsigned char* data,unsigned int *len);

         private:

            fax_demod(float quad_rate, float black_freq, float white_freq,float lpm,float ioc);
            ~fax_demod();

            void set_input_filter(void);
            void set_pixel_filter(void);
            void set_quad(void);

            /* parameters */
            float   d_quad_rate;   /*! Sample rate */
            float   d_black_freq;   /*! Black frequency */
            float   d_white_freq;   /*! White frequency */
            float   d_lpm;   /*! Lines per minute */
            float   d_ioc;   /*! Index of cooperation */

            /* Input filter */
            gr::filter::freq_xlating_fir_filter_ccf::sptr input_filter;

            /* quadrature demodulator */
            gr::analog::quadrature_demod_cf::sptr   quad;

            /* fax decoder */
            gr::fax::fax_decoder::sptr fax_decoder;

            /* fax line storage */
            gr::fax::fax_store::sptr fax_store;

            /* internal variable */
            float   fc, delta, fmin, fmax;
            float   pixel_rate;
            int     dec;
      };

   }
}
#endif // FAX_DEMOD_H
