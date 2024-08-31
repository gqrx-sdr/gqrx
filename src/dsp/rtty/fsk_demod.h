/*
 * fsk/afsk demodulator block header
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * fsk/afsk demodulator block header
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

#ifndef _FSK_DEMOD_H_
#define _FSK_DEMOD_H_

#include <gnuradio/sync_decimator.h>

namespace gr {
   namespace rtty {
      class fsk_demod : virtual public gr::sync_decimator {
         public:
#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<fsk_demod> sptr;
#else
            typedef std::shared_ptr<fsk_demod> sptr;
#endif

            static sptr make(float sample_rate,unsigned int decimation,float symbol_rate, float mark_freq,float space_freq,float threshold=6.0f,float cd_len=6.0f);

            virtual    int work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) = 0;

            virtual void set_sample_rate(float sample_rate) = 0;
            virtual float sample_rate() const = 0;

            virtual void set_decimation(unsigned int decimation) = 0;
            virtual int decimation() const = 0;

            virtual void set_symbol_rate(float symbol_rate) = 0;
            virtual float symbol_rate() const = 0;

            virtual void set_mark_freq(float mark_freq) = 0;
            virtual float mark_freq() const = 0;

            virtual void set_space_freq(float space_freq) = 0;
            virtual float space_freq() const = 0;

            virtual void set_threshold(float threshold) = 0;
            virtual float threshold() const = 0;

            virtual void set_cd_len(float cd_len) = 0;
            virtual float cd_len() const = 0;

            virtual void set_bandwidth(float bw) = 0;
            virtual float bandwidth() const = 0;

            virtual void set_transwidth(float trw) = 0;
            virtual float transwidth() const = 0;

            virtual void set_filterlen(float filterlen) = 0;
            virtual float filterlen() const = 0;

      };
   } // namespace rtty
} // namespace gr

#endif // _FSK_DEMOD_H_
