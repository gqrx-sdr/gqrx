/*
 * fax demodulator hier block implementation
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
#include <dsp/fax/fax_demod.h>

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

      /* Create a new instance of fax_demod and return a boost shared_ptr. */
      fax_demod::sptr fax_demod::make(float quad_rate, float black_freq, float white_freq,float lpm, float ioc) {
         return gnuradio::get_initial_sptr(new fax_demod(quad_rate, black_freq, white_freq, lpm, ioc));
      }

      fax_demod::fax_demod(float quad_rate, float black_freq, float white_freq, float lpm, float ioc)
         : gr::hier_block2 ("fax_demod",
               gr::io_signature::make (1, 1, sizeof (gr_complex)),
               gr::io_signature::make (0, 0, sizeof (char))),
         d_quad_rate(quad_rate),
         d_black_freq(black_freq),
         d_white_freq(white_freq),
         d_lpm(lpm),
         d_ioc(ioc) {

            std::vector<gr_complex> ctaps;
            std::vector<float> ftaps;

            delta = (d_white_freq - d_black_freq);
            fc = (d_white_freq + d_black_freq)/2;
            if (!delta) delta = 800;

            /* Input Filter */
            //ftaps = gr::filter::firdes::low_pass(1.0f, d_quad_rate,  abs(delta/2) ,2000);
            ftaps.clear();
            ftaps.resize(1);
            ftaps[0] = 1;
            input_filter = gr::filter::freq_xlating_fir_filter_ccf::make(1,ftaps,(double)fc,(double)d_quad_rate);

            /* quadrature demodulator */
            quad = gr::analog::quadrature_demod_cf::make(d_quad_rate / (2.0f * M_PIf * delta/2));

            /* fax decoder */
            fax_decoder = gr::fax::fax_decoder::make(d_quad_rate,d_lpm,d_ioc);

            /* fax storage */
            fax_store = gr::fax::fax_store::make(120); // 1 minute circular buffer at 120lpm

            /* connect blocks */
            connect(self(), 0, input_filter, 0);
            connect(input_filter, 0, quad, 0);
            connect(quad, 0, fax_decoder, 0);
            connect(fax_decoder, 0, fax_store, 0);
         }

      fax_demod::~fax_demod ()
      {

      }

      void fax_demod::set_input_filter(void) {
         std::vector<float> ftaps;

         delta = (d_white_freq - d_black_freq);
         fc = (d_white_freq + d_black_freq)/2;
         if (!delta) {
            delta=800;
         }

         //ftaps = gr::filter::firdes::low_pass(1.0f, d_quad_rate,  abs(delta/2) ,2000);
         ftaps.clear();
         ftaps.resize(1);
         ftaps[0] = 1;
         input_filter->set_taps(ftaps);
         input_filter->set_center_freq((double)fc);
      }

      void fax_demod::set_quad(void) {
         quad->set_gain(d_quad_rate / (2.0f * M_PIf * delta/2));
      }

      void  fax_demod::set_quad_rate(float quad_rate) {
         d_quad_rate = quad_rate;

         set_input_filter();
         set_quad();
         fax_decoder->set_sample_rate(d_quad_rate);

      }

      float fax_demod::quad_rate() const {
         return d_quad_rate;
      }

      void fax_demod::set_black_freq(float black_freq) {
         d_black_freq = black_freq;

         set_input_filter();
         set_quad();
      }

      float fax_demod::black_freq() const {
         return d_black_freq;
      }

      void fax_demod::set_white_freq(float white_freq) {
         d_white_freq = white_freq;

         set_input_filter();
         set_quad();
      }

      float fax_demod::white_freq() const {
         return d_white_freq;
      }

      void fax_demod::set_lpm(float lpm) {
         if (!lpm)
            return;
         d_lpm = lpm;
         set_quad();
         fax_decoder->set_lpm(d_lpm);
      }

      float fax_demod::lpm() const {
         return fax_decoder->lpm();
      }

      void fax_demod::set_ioc(float ioc) {
         if (!ioc)
            return;
         d_ioc = ioc;
         set_quad();
         fax_decoder->set_ioc(d_ioc);
      }

      float fax_demod::ioc() const {
         return fax_decoder->ioc();
      }

      void fax_demod::reset(void) {
         fax_store->reset();
         fax_decoder->reset();
      }

      int fax_demod::get_data(unsigned char * data,unsigned int *len) {

         return fax_store->get_data(data,len);
      }

      void fax_demod::set_decoder_state(int state) {
         fax_decoder->set_state(state);
      }

      int fax_demod::decoder_state() const {
         return fax_decoder->state();
      }

   }
}
