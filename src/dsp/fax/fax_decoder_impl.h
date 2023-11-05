/*
 * fax decoder block implementation header
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
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

#ifndef FAX_DECODER_IMPL_H
#define FAX_DECODER_IMPL_H

#include "fax_decoder.h"
#include <gnuradio/fft/goertzel.h>
#include <mutex>

namespace gr {
   namespace fax {

      class fax_decoder_impl : public fax_decoder
      {
         public:
            fax_decoder_impl(float sample_rate, float lpm, float ioc);
            ~fax_decoder_impl();

            void forecast(int noutput_items, gr_vector_int &ninput_items_required);

            int general_work(int noutput_items,
                  gr_vector_int &ninput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items);

            void set_sample_rate(float sample_rate);
            float sample_rate() const;

            void set_lpm(float lpm);
            float lpm() const;

            void set_ioc(float ioc);
            float ioc() const;

            void set_state(int state);
            int state() const;

            void reset();

         private:
            enum fax_decoder_impl_state {
               FAX_RESET = 0,    // Reset state machine
               FAX_WAIT_START, // Wait for start tone
               FAX_WAIT_WHITE,    // Wait for Black to white transition
               FAX_MEASURE_WHITE, // Measure White Len
               FAX_MEASURE_BLACK, // Measure Black Len
               FAX_GET_LINES // Get Lines until stop tone
            };

            std::mutex mutex;

            // Properties
            float d_sample_rate;
            float d_lpm;
            float d_ioc;


            float pixel_len;
            float line_len;
            float line_pos;
            float line_start;
            float noise_len;
            int phasing;
            float phasing_len;
            float phasing_start;
            int phasing_count;
            bool bit;
            float in_count;

            enum fax_decoder_impl_state d_state;

            gr::fft::goertzel tone_300; // Start at ioc = 576
            gr::fft::goertzel tone_675; // Start at ioc = 288
            gr::fft::goertzel tone_450; // Stop
      };
   }
}

#endif // FAX_DECODER_H
