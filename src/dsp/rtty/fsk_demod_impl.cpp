/*
 * fsk/afsk demodulator block implementation
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
#include <gnuradio/filter/firdes.h>
#include <gnuradio/math.h>
#include <complex>
#include <volk/volk.h>
#include "fsk_demod_impl.h"

#ifndef M_PIf
# define M_PIf          3.14159265358979323846f /* pi */
#endif



#define FSK_DEMOD_NTAPS (d_filterlen * d_sample_rate/d_symbol_rate)                      // Filter on 1 symbol

#define FSK_DEMOD_BP_ATT   (FSK_DEMOD_NTAPS*22.0f*d_trw/d_sample_rate)       // Attenuation

#define FSK_DEMOD_RATE  (d_sample_rate/d_decimation)                            // output rate
#define FSK_SYMBOL_LEN  (FSK_DEMOD_RATE/d_symbol_rate)

namespace gr {
   namespace rtty {

      fsk_demod_impl::fsk_demod_impl(float sample_rate, unsigned int decimation, float symbol_rate, float mark_freq, float space_freq,float threshold,float cd_len) :
         gr::sync_decimator("fsk_demod",
               gr::io_signature::make(1, 1, sizeof(gr_complex)),
               gr::io_signature::make(1, 5, sizeof(float)),
               decimation ),
#if GNURADIO_VERSION < 0x030900
         d_mark_fir(1,{}),
         d_space_fir(1,{}),
#else
         d_mark_fir({}),
         d_space_fir({}),
#endif
         d_sample_rate(sample_rate),
         d_decimation(decimation),
         d_symbol_rate(symbol_rate),
         d_mark_freq(mark_freq),
         d_space_freq(space_freq),
         d_threshold(threshold),
         d_cd_len(cd_len) {

            d_bw = d_symbol_rate*2;
            d_trw = d_symbol_rate/2;
            d_filterlen = 1.0f;

            d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
            set_history(d_tone_filter_taps.size());

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            count_noise = 0;
            count_mark=0;
            count_space=0;
	    noise_count = 0;
	    cd_count = 0;
	    cd = false;
	    last_noise = true;
	    last_bit = false;

            set_threshold(threshold);
            set_cd_len(d_cd_len);
         }

      fsk_demod::sptr fsk_demod::make(float sample_rate, unsigned int decimation, float symbol_rate, float mark_freq, float space_freq,float threshold,float cd_len) {
         return gnuradio::get_initial_sptr (new fsk_demod_impl(sample_rate, decimation, symbol_rate, mark_freq,space_freq,threshold,cd_len));
      }

      fsk_demod_impl::~fsk_demod_impl() {
      }

      void fsk_demod_impl::set_sample_rate(float sample_rate) {
         std::lock_guard<std::recursive_mutex> lock(d_mutex);

         d_sample_rate = sample_rate;

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);

         count_noise = 0;
         count_mark=0;
         count_space=0;
	 cd = false;

         set_cd_len(d_cd_len);
      }

      float fsk_demod_impl::sample_rate() const {
         return d_sample_rate;
      }

      void fsk_demod_impl::set_decimation(unsigned int decimation) {
         std::lock_guard<std::recursive_mutex> lock(d_mutex);
         d_decimation = decimation;

         gr::sync_decimator::set_decimation(decimation);

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);

	 while (!last_samples.empty())
		 last_samples.pop();

         count_noise = 0;
         count_mark=0;
         count_space=0;

         set_cd_len(d_cd_len);
      }

      int fsk_demod_impl::decimation() const {
         return d_decimation;
      }

      void fsk_demod_impl::set_symbol_rate(float symbol_rate) {
         std::lock_guard<std::recursive_mutex> lock(d_mutex);

         d_symbol_rate = symbol_rate;

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);

	 while (!last_samples.empty())
		 last_samples.pop();

         count_noise = 0;
         count_mark=0;
         count_space=0;

         set_cd_len(d_cd_len);
      }

      float fsk_demod_impl::symbol_rate() const {
         return d_symbol_rate;
      }

      void fsk_demod_impl::set_mark_freq(float mark_freq) {
         std::lock_guard<std::recursive_mutex> lock(d_mutex);

         std::vector<gr_complex> ctaps(d_tone_filter_taps.size());
         float fwT0 = 2* M_PIf * (mark_freq/d_sample_rate);


         for (unsigned int i=0; i< d_tone_filter_taps.size(); i++) {
            ctaps[i] = d_tone_filter_taps[i] * exp(gr_complex(0, i * fwT0));
         }

         d_mark_fir.set_taps(ctaps);
         d_mark_freq = mark_freq;
      }

      float fsk_demod_impl::mark_freq() const {
         return d_mark_freq;
      }

      void fsk_demod_impl::set_space_freq(float space_freq) {
         std::lock_guard<std::recursive_mutex> lock(d_mutex);

         std::vector<gr_complex> ctaps(d_tone_filter_taps.size());
         float fwT0 = 2* M_PIf * (space_freq/d_sample_rate);

         for (unsigned int i=0; i< d_tone_filter_taps.size(); i++) {
            ctaps[i] = d_tone_filter_taps[i] * exp(gr_complex(0, i * fwT0));
         }

         d_space_fir.set_taps(ctaps);
         d_space_freq = space_freq;

      }

      float fsk_demod_impl::space_freq() const {
         return d_space_freq;
      }

      void fsk_demod_impl::set_threshold(float threshold) {
         d_threshold = threshold;
         threshold_mark = pow(10,(d_threshold/10));
         threshold_space = 1.0f/threshold_mark;
      }

      float fsk_demod_impl::threshold() const {
         return d_threshold;
      }

      void fsk_demod_impl::set_cd_len(float cd_len) {
         d_cd_len = cd_len;
         cd_count = roundf(FSK_SYMBOL_LEN*d_cd_len);
      }

      float fsk_demod_impl::cd_len() const {
         return d_cd_len;
      }

      // band pass filter width
      void fsk_demod_impl::set_bandwidth(float bw) {
         d_bw = bw;

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);
      }
      float fsk_demod_impl::bandwidth() const {
         return d_bw;
      }

      // band pass filter transition width
      void fsk_demod_impl::set_transwidth(float trw) {
         d_trw = trw;

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);
      }

      float fsk_demod_impl::transwidth() const {
         return d_trw;
      }

      // Band pass filter len in number of symbo
      void fsk_demod_impl::set_filterlen(float filterlen) {

         d_filterlen = filterlen;

         d_tone_filter_taps = gr::filter::firdes::low_pass_2((double)1.0f, (double)d_sample_rate, (double)d_bw/2, (double)d_trw, (double)FSK_DEMOD_BP_ATT);
         set_history(d_tone_filter_taps.size());

         set_mark_freq(d_mark_freq);
         set_space_freq(d_space_freq);
      }

      float fsk_demod_impl::filterlen() const {
         return d_filterlen;
      }

      int fsk_demod_impl::work (int noutput_items,
            gr_vector_const_void_star& input_items,
            gr_vector_void_star& output_items) {

         std::lock_guard<std::recursive_mutex> lock(d_mutex);
         float * bit = reinterpret_cast<float*>(output_items[0]);
         const gr_complex * in = reinterpret_cast<const gr_complex*>(input_items[0]);
         int al = volk_get_alignment();
         unsigned int i,j;
         std::vector<float> vcarrier_detect(al);
         std::vector<gr_complex> vmark(noutput_items + al);
         std::vector<gr_complex> vspace(noutput_items + al);
         std::vector<float> vmark_mag(al);
         std::vector<float> vspace_mag(al);
         std::vector<float> vsnr(al);
         float * carrier_detect;
         gr_complex * mark;
         gr_complex * space;
         float * mark_mag;
         float * space_mag;
         float * snr;
         char current_sample;

         mark =   (gr_complex*)((((size_t)vmark.data())+(al-1)) & ~(al-1));
         space =  (gr_complex*)((((size_t)vspace.data())+(al-1)) & ~(al-1));

         // Output carrier detection if connected
         if (output_items.size()>1 && output_items[1]!=NULL)
            carrier_detect = reinterpret_cast<float*>(output_items[1]);
         else {
            vcarrier_detect.resize(noutput_items + al);
            carrier_detect =  (float*)((((size_t)vcarrier_detect.data())+(al-1)) & ~(al-1));
         }

         // Output Mark power if connected
         if (output_items.size()>2 && output_items[2]!=NULL)
            mark_mag = reinterpret_cast<float*>(output_items[2]);
         else {
            vmark_mag.resize(noutput_items + al);
            mark_mag =  (float*)((((size_t)vmark_mag.data())+(al-1)) & ~(al-1));
         }

         // Output space power if connected
         if (output_items.size()>3 && output_items[3]!=NULL)
            space_mag = reinterpret_cast<float*>(output_items[3]);
         else {
            vspace_mag.resize(noutput_items + al);
            space_mag =  (float*)((((size_t)vspace_mag.data())+(al-1)) & ~(al-1));
         }

         // Output signal to noise ration in dB if connected (positive for mark tone, negative for space tone)
         if (output_items.size()>4 && output_items[4]!=NULL)
            snr = reinterpret_cast<float*>(output_items[4]);
         else {
            vsnr.resize(noutput_items + al);
            snr =  (float*)((((size_t)vsnr.data())+(al-1)) & ~(al-1));
         }

         // Filter tones and decimate
         for (i=0,j=0;i<(unsigned int)noutput_items;i++,j+=d_decimation) {
            mark[i] = d_mark_fir.filter(&in[j]);
            space[i] = d_space_fir.filter(&in[j]);
         }

         // Get instantaneous tones power
         volk_32fc_magnitude_squared_32f(mark_mag,mark,noutput_items);
         volk_32fc_magnitude_squared_32f(space_mag,space,noutput_items);

         // Calculate mark to space ratio (snr)
         volk_32f_x2_divide_32f(snr,mark_mag,space_mag,noutput_items);

         for (i=0;i<(unsigned int)noutput_items;i++) {
            // Calculate current sample state (mark, no tone, or space)
            if (snr[i] >= threshold_mark)
               current_sample = 1;
            else if (snr[i] <= threshold_space)
               current_sample = -1;
            else
               current_sample = 0;

            // Count samples state on one symbol len
            if (last_samples.size() == FSK_SYMBOL_LEN) {
               switch (last_samples.front()) {
                  case 1 : if (count_mark) count_mark--; break;
                  case 0 : if (count_noise) count_noise--; break;
                  case -1: if (count_space) count_space--; break;
               }
               last_samples.pop();
            }

            switch (current_sample) {
               case 1 : if (count_mark<FSK_SYMBOL_LEN) count_mark++; break;
               case 0 : if (count_noise<FSK_SYMBOL_LEN) count_noise++; break;
               case -1 : if (count_space<FSK_SYMBOL_LEN) count_space++; break;
            }
            last_samples.push(current_sample);

            // Take decision
            if (count_mark>count_noise && count_mark>count_space) {
               last_bit = true;
               last_noise = false;
               bit[i] = 1;
            }
            else if (count_space>count_noise && count_space>count_mark) {
               last_bit = false;
               last_noise = false;
               bit[i] = -1;
            }
            else if (count_noise>count_mark && count_noise>count_space) {
               last_noise = true;
               if (last_bit)
                  bit[i] = -1;
               else
                  bit[i] = 1;
            }
            else if (last_noise) {
               if (last_bit)
                  bit[i] = -1;
               else
                  bit[i] = 1;
            } else {
               if (last_bit)
                  bit[i] = 1;
               else
                  bit[i] = -1;
            }

            // Carrier detection
            if (!last_noise) {
               if (signal_count < cd_count) {
                  noise_count=0;
                  signal_count++;
               } else cd = true;
            } else {
               if (noise_count < cd_count) {
                  signal_count=0;
                  noise_count++;
               } else cd = false;
            }
            carrier_detect[i] = cd?1:0;
         }

         // Output signal to noise ratio as dB (positive for mark, negative for space)
         if (output_items.size()>3 && output_items[3]!=NULL) { // convert to dB if connected
            volk_32f_log2_32f(snr,snr,noutput_items);
            volk_32f_s32f_multiply_32f(snr,snr,3.0f,noutput_items);
         }

         return noutput_items;
      }

   } /* namespace rtty */
} /* namespace gr */
