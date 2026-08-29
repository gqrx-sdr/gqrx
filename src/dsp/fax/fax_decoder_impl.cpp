/*
 * fax decoder block implementation
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

#include <gnuradio/io_signature.h>
#include <volk/volk.h>
#include "fax_decoder_impl.h"

#if defined(DEBUG) && 0
#include <stdio.h>
#define D(...)    do { fprintf(stderr,__VA_ARGS__); } while (0)
#else
#define D(...)
#endif

#ifndef M_PIf
# define M_PIf          3.14159265358979323846f /* pi */
#endif

namespace gr {
   namespace fax {

      fax_decoder::sptr fax_decoder::make(float sample_rate, float lpm, float ioc) {
         return gnuradio::get_initial_sptr(new fax_decoder_impl(sample_rate, lpm, ioc));
      }

      fax_decoder_impl::fax_decoder_impl(float sample_rate, float lpm, float ioc) :
         gr::block("fax_decoder",
               gr::io_signature::make(1, 1, sizeof(float)),
               gr::io_signature::make(1, 1, sizeof(float))),
         d_sample_rate(sample_rate),
         d_lpm(lpm),
         d_ioc(ioc),
         tone_300(sample_rate,4*sample_rate * 60.0f/(lpm),sample_rate/(roundf(sample_rate/300))),
         tone_675(sample_rate,4*sample_rate * 60.0f/(lpm),sample_rate/(roundf(sample_rate/675))),
         tone_450(sample_rate,4*sample_rate * 60.0f/(lpm),sample_rate/(roundf(sample_rate/450))) {

            line_len = ((d_sample_rate *60.0f) / d_lpm);
            pixel_len = line_len/(d_ioc*(float)M_PIf);
            d_state = FAX_RESET;
            in_count=0;
         }

      fax_decoder_impl::~fax_decoder_impl() {
      }

      void fax_decoder_impl::set_sample_rate(float sample_rate) {
         std::lock_guard<std::mutex> lock(mutex);

         d_sample_rate = sample_rate;
         line_len = (d_sample_rate *60.0f / d_lpm);
         pixel_len = line_len/(d_ioc*M_PIf);
         in_count=0;
      }

      float fax_decoder_impl::sample_rate() const {
         return d_sample_rate;
      }

      void fax_decoder_impl::set_lpm(float lpm) {
         std::lock_guard<std::mutex> lock(mutex);

         d_lpm = lpm;
         line_len = (d_sample_rate *60.0f / d_lpm);
         pixel_len = line_len/(d_ioc*M_PIf);
         in_count=0;
      }

      float fax_decoder_impl::lpm() const {
         return d_lpm;
      }

      void fax_decoder_impl::set_ioc(float ioc) {
         std::lock_guard<std::mutex> lock(mutex);

         d_ioc = ioc;
         line_len = (d_sample_rate *60.0f / d_lpm);
         pixel_len = line_len/(d_ioc*M_PIf);
      }

      float fax_decoder_impl::ioc() const {
         return d_ioc;
      }

      void fax_decoder_impl::set_state(int state) {
         std::lock_guard<std::mutex> lock(mutex);

         switch (state) {
            default:
            case 0: d_state = FAX_RESET;in_count=0;D("FAX_RESET\n");break;
            case 1: d_state = FAX_WAIT_START;D("FAX_WAIT_START\n");break;
            case 2: d_state = FAX_WAIT_WHITE;D("FAX_WAIT_WHITE\n");break;
            case 3: d_state = FAX_MEASURE_WHITE;D("FAX_MEASURE_WHITE\n");break;
            case 4: d_state = FAX_MEASURE_BLACK;D("FAX_MEASURE_BLACK\n");break;
            case 5: d_state = FAX_GET_LINES;D("FAX_GET_LINES\n");break;
         };
      }

      int fax_decoder_impl::state() const {
         return d_state;
      }


      void fax_decoder_impl::reset() {
         std::lock_guard<std::mutex> lock(mutex);
         line_len = (d_sample_rate *60.0f / d_lpm);
         pixel_len = line_len/(d_ioc*M_PIf);
         d_state = FAX_RESET;
         in_count=0;
         D("FAX_RESET\n");
      }

      void fax_decoder_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
         std::lock_guard<std::mutex> lock(mutex);

         ninput_items_required[0] = roundf((noutput_items *pixel_len))+1;
      }

      int fax_decoder_impl::general_work (int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items) {
         std::lock_guard<std::mutex> lock(mutex);

         const float* in = &((reinterpret_cast<const float*>(input_items[0]))[(int)(0)]);
         float *out = reinterpret_cast<float*>(output_items[0]);
         int out_count = 0;
         float InAcc, tone3,tone4=0,tone6;

         while( (out_count < (noutput_items)) && (roundf(in_count) < (ninput_items[0]-(int)roundf(pixel_len)))) {
            volk_32f_accumulator_s32f(&InAcc,&in[(int)roundf(in_count)],(int)roundf(pixel_len));
            InAcc/=pixel_len;

            if (InAcc>0.8f)
               bit = true;
            else if (InAcc<-0.8f)
               bit = false;

            switch (d_state) {
               case FAX_RESET:
                  line_pos=0;
                  in_count=0;
                  noise_len=0;
                  line_start=0;
                  line_len = (d_sample_rate *60.0f / d_lpm);
                  pixel_len = line_len/(d_ioc*M_PIf);
                  bit = false;
                  phasing=3;
                  phasing_count=0;
                  phasing_len=0;
                  phasing_start=0;
                  tone_300.set_params(d_sample_rate,(int)roundf(line_len),300);
                  tone_675.set_params(d_sample_rate,(int)roundf(line_len),675);
                  tone_450.set_params(d_sample_rate/pixel_len,(int)roundf(line_len/pixel_len),450);
                  d_state = FAX_WAIT_START;
                  D("FAX_WAIT_START\n");
                  break;
               case FAX_WAIT_START:
                  if (tone_300.ready()) {
                     tone3=abs(tone_300.output());
                     D("tone 300 : %f\n",tone3);
                     tone_300.set_params(d_sample_rate,(int)roundf(line_len),d_sample_rate/(roundf(d_sample_rate/300)));
                     if (tone3>0.25f) { //300Hz tone detected
                        noise_len = 0;
                        d_state = FAX_WAIT_WHITE;
                        D("FAX_WAIT_WHITE @300Hz\n");
                        bit = false;
                        consume_each (ninput_items[0]);
                        return 0;
                     }
                  }
                  if (tone_675.ready()) {
                     tone6=abs(tone_675.output());
                     D("tone 675 : %f\n",tone6);
                     tone_675.set_params(d_sample_rate,(int)roundf(line_len),d_sample_rate/(roundf(d_sample_rate/675)));
                     if (tone6>0.25f) { //675Hz tone detected
                        noise_len = 0;
                        d_state = FAX_WAIT_WHITE;
                        D("FAX_WAIT_WHITE @675Hz\n");
                        bit = false;
                        consume_each (ninput_items[0]);
                        return 0;
                     }
                  }
                  tone_300.input(in[(int)roundf(in_count)]);
                  tone_675.input(in[(int)roundf(in_count)]);
                  in_count++;
                  break;
               case FAX_WAIT_WHITE:    // Wait for Black to white transition
                  in_count++;
                  if (!bit) { // Black
                     noise_len = 0;
                  } else    { // White
                     noise_len++;
                     if (noise_len >= (10.0f*pixel_len)) { // Not noise : Transition to white
                        noise_len = 0;
                        line_pos = 0;
                        line_start = 0;
                        d_state = FAX_MEASURE_WHITE;
                        D("FAX_MEASURE_WHITE\n");
                     }
                  }

                  break;
               case FAX_MEASURE_WHITE:    // Measure White Len
                  in_count ++;
                  line_pos++;
                  if (bit) { // White
                     noise_len = 0;
                  } else { // Black
                     noise_len++;
                     if (noise_len >= (10.0f*pixel_len)) { // Not noise : Transition to black
                        //in_count -= noise_len;
                        //line_pos -= noise_len;
                        noise_len = 0;
                        if ((!line_start && (line_pos < (0.04f*line_len) || line_pos > (0.06f*line_len))) // White pulse too short
                              || (line_start && (line_pos < (line_start - 0.01f*line_len) || line_pos > (line_start + 0.01f*line_len)))) {
                           noise_len=0;
                           bit = false;
                           d_state = FAX_WAIT_WHITE;
                           D("FAX_WAIT_WHITE\n");
                        }
                        else {

                           line_start = line_pos;
                           d_state = FAX_MEASURE_BLACK;
                           D("FAX_MEASURE_BLACK\n");
                        }
                     }
                  }
                  break;
               case FAX_MEASURE_BLACK:    // Measure Black Len
                  in_count ++;
                  line_pos++;
                  if (!bit) { // Black
                     noise_len = 0;
                  } else { // White
                     noise_len++;
                     if (noise_len >= (10*pixel_len)) { // Not noise : Transition to white
                        //in_count -= noise_len;
                        //line_pos -= noise_len;
                        if (line_pos < (line_len*0.99f) || line_pos > (line_len*1.01f)) // Black line too short
                        {
                           noise_len = 0;
                           d_state = FAX_MEASURE_WHITE;
                           D("FAX_WAIT_WHITE\n");
                        }
                        else
                        {
                           phasing_count++;
                           phasing_len+=line_pos;
                           line_pos = 0;
                           phasing_start+=line_start;
                           line_start = phasing_start/phasing_count;
                           //line_len = phasing_len/phasing_count;
                           //pixel_len = line_len / (d_ioc*2*M_PIf);
                           if (phasing) {
                              noise_len = 0;
                              phasing--;
                              d_state = FAX_MEASURE_WHITE;
                              D("FAX_MEASURE_WHITE\n");
                           } else {
                              d_state = FAX_GET_LINES;
                              D("FAX_GET_LINES\n");
                           }
                        }
                     }
                  }
                  break;
               case FAX_GET_LINES: // Get the fax until 450Hz tone
                  if (tone_450.ready()) {
                     tone4=abs(tone_450.output());
                     D("tone 450 : %f\n",tone4);
                     tone_450.set_params(d_sample_rate/pixel_len,(int)roundf(line_len/pixel_len),450);
                     if (tone4 > 0.25f) { // 450Hz tone detected
                        // End of transmition
                        *out = +INFINITY; // End of image
                        out++;
                        out_count++;
                        D("FAX_RESET\n");
                        d_state = FAX_RESET;
                        break;
                     }
                  }
                  tone_450.input(InAcc);

                  *out=InAcc;
                  out++;
                  out_count++;

                  in_count+=pixel_len;
                  line_pos+=pixel_len;

                  if ( line_pos >= line_len) { // End of line
                     line_pos -= line_len;
                     *out = -INFINITY; // End of line marker
                     out++;
                     out_count++;
                  }
                  break;

            }
         }

         consume_each ((int)roundf(in_count));
         in_count -= roundf(in_count);

         return (out_count);
      }

   }
}
