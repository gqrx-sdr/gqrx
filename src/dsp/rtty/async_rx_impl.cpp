/*
 * asynchronous receiver block implementation
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
#include "async_rx_impl.h"

namespace gr {
   namespace rtty {

      async_rx::sptr async_rx::make(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) {
         return gnuradio::get_initial_sptr(new async_rx_impl(sample_rate, bit_rate, word_len, parity));
      }

      async_rx_impl::async_rx_impl(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) :
         gr::block("async_rx",
               gr::io_signature::make(1, 2, sizeof(float)),
               gr::io_signature::make(1, 1, sizeof(unsigned char))),
         d_sample_rate(sample_rate),
         d_bit_rate(bit_rate),
         d_word_len(word_len),
         d_parity(parity) {
            bit_len = (sample_rate / bit_rate);
            state = ASYNC_WAIT_IDLE;
            cd = false;
	    send_esc = false;
	    send_sot = false;
	    send_eot = false;
         }

      async_rx_impl::~async_rx_impl() {
      }

      void async_rx_impl::set_word_len(char word_len) {
         std::lock_guard<std::mutex> lock(d_mutex);

         d_word_len = word_len;
      }

      char async_rx_impl::word_len() const {
         return d_word_len;
      }

      void async_rx_impl::set_sample_rate(float sample_rate) {
         std::lock_guard<std::mutex> lock(d_mutex);

         d_sample_rate = sample_rate;
         bit_len = (sample_rate / d_bit_rate);
      }

      float async_rx_impl::sample_rate() const {
         return d_sample_rate;
      }

      void async_rx_impl::set_bit_rate(float bit_rate) {
         std::lock_guard<std::mutex> lock(d_mutex);

         d_bit_rate = bit_rate;
         bit_len = (d_sample_rate / bit_rate);
      }

      float async_rx_impl::bit_rate() const {
         return d_bit_rate;
      }

      void async_rx_impl::set_parity(enum async_rx::async_rx_parity parity) {
         std::lock_guard<std::mutex> lock(d_mutex);

         d_parity = parity;
      }

      enum async_rx::async_rx_parity async_rx_impl::parity() const {
         return d_parity;
      }

      void async_rx_impl::reset() {
         std::lock_guard<std::mutex> lock(d_mutex);

         state = ASYNC_WAIT_IDLE;
	 cd = false;
	 send_esc = false;
	 send_sot = false;
	 send_eot = false;
      }

      void async_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
         std::lock_guard<std::mutex> lock(d_mutex);

         ninput_items_required[0] = (noutput_items * (d_word_len+2+(d_parity==ASYNC_RX_PARITY_NONE?0:1))+1) * bit_len;
      }

      int async_rx_impl::general_work (int noutput_items,
            gr_vector_int &ninput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items) {

         std::lock_guard<std::mutex> lock(d_mutex);
         int in_count = 0;
         int out_count = 0;
         const float *in = reinterpret_cast<const float*>(input_items[0]);
         const float *carrier_detect;
         unsigned char *out = reinterpret_cast<unsigned char*>(output_items[0]);

         if (input_items.size()>1 && input_items[1]!=NULL)
            carrier_detect = reinterpret_cast<const float*>(input_items[1]);
         else    carrier_detect = NULL;

         while( (out_count < noutput_items) && (in_count < (ninput_items[0]-(bit_len)))) {

            if (in[in_count] >0)
               bit = true;
            else bit = false;

            if (carrier_detect && in_count < ninput_items[1]) { // carrier detect signal connected
               if (send_esc) {
                  *out = ASYNC_RX_ESC_CHAR;
                  out++;
                  out_count++;
                  send_esc = false;
                  continue;
               } else if (send_sot) {
                  *out = ASYNC_RX_SOT_CHAR;
                  out++;
                  out_count++;
                  send_sot = false;
                  continue;
               } else if (send_eot) {
                  *out = ASYNC_RX_EOT_CHAR;
                  out++;
                  out_count++;
                  send_eot = false;
                  continue;
               }
               if (!cd && carrier_detect[in_count]>0) {
                  cd = true;
                  // send ESC+SOT
                  *out = ASYNC_RX_ESC_CHAR;
                  send_sot = true;
                  out++;
                  out_count++;
                  continue;
               } else if (cd && carrier_detect[in_count] <=0) {
                  cd = false;
                  // send ESC+EOT
                  *out = ASYNC_RX_ESC_CHAR;
                  send_eot = true;
                  out++;
                  out_count++;
                  continue;
               }
            } else {
               cd = true;
               send_esc = false;
               send_sot = false;
               send_eot = false;
            }


            switch (state) {
               case ASYNC_IDLE:    // Wait for MARK to SPACE transition
                  if (!bit) { // transition detected
                     state = ASYNC_CHECK_START;
                     word_pos=0;
                  }
                  break;
               case ASYNC_CHECK_START:    // Check start bit for half a bit
                  word_pos++;
                  if (!bit) { // Space
                     if (word_pos>=bit_len/2) { // start bit verified
                        state = ASYNC_GET_BIT;
                        bit_pos = 0;
                        bit_count = 0;
                        word = 0;
                     }
                  } else { // Noise detection on start
                     state = ASYNC_IDLE;
                  }
                  break;
               case ASYNC_GET_BIT:
                  word_pos++;
                  if (word_pos>=(((float)bit_pos+1.5f)*bit_len)) { // sample at center of bit
                     if (bit) {
                        word |= 1<<bit_pos;
                        bit_count++;
                     }
                     bit_pos++;
                     if (bit_pos == d_word_len) {
                        if (d_parity == ASYNC_RX_PARITY_NONE)
                           state = ASYNC_CHECK_STOP;
                        else
                           state = ASYNC_CHECK_PARITY;
                     }
                  }
                  break;
               case ASYNC_CHECK_PARITY: // Check parity bit
                  word_pos++;
                  if (word_pos>=(((float)bit_pos+1.5f)*bit_len)) { // sample at center of bit
                     bit_pos++;
                     switch (d_parity) {
                        default:
                        case ASYNC_RX_PARITY_NONE:
                           state = ASYNC_CHECK_STOP;
                           break;
                        case ASYNC_RX_PARITY_ODD:
                           if ((!bit && (bit_count&1)) || (bit && !(bit_count&1))) {
                              state = ASYNC_CHECK_STOP;
                           }
                           else {
                              if (bit)
                                 state = ASYNC_IDLE;
                              else
                                 state = ASYNC_WAIT_IDLE;
                           }
                           break;
                        case ASYNC_RX_PARITY_EVEN:
                           if ((!bit && !(bit_count&1)) || (bit && (bit_count&1))) {
                              state = ASYNC_CHECK_STOP;
                           }
                           else {
                              if (bit)
                                 state = ASYNC_IDLE;
                              else
                                 state = ASYNC_WAIT_IDLE;
                           }
                           break;
                        case ASYNC_RX_PARITY_MARK:
                           if (bit) {
                              state = ASYNC_CHECK_STOP;
                           }
                           else {
                              state = ASYNC_WAIT_IDLE;
                           }
                           break;
                        case ASYNC_RX_PARITY_SPACE:
                           if (!bit) {
                              state = ASYNC_CHECK_STOP;
                           }
                           else {
                              state = ASYNC_IDLE;
                           }
                           break;
                        case ASYNC_RX_PARITY_DONTCARE:
                           state = ASYNC_CHECK_STOP;
                           break;
                     }
                  }
                  break;
               case ASYNC_CHECK_STOP: // Check stop bit
                  word_pos++;
                  if (word_pos>=(((float)bit_pos+1.5f)*bit_len)) { // sample at center of bit
                     bit_pos++;
                     if (bit) { // Stop bit verified
                        if (cd) {
                           *out = word;
                           out++;
                           out_count++;

                           if (word == ASYNC_RX_ESC_CHAR)
                              send_esc = true;
                        }
                        state = ASYNC_IDLE;
                     }
                     else
                        state = ASYNC_WAIT_IDLE;
                  }
                  break;
               default:
               case ASYNC_WAIT_IDLE:    // Wait for SPACE to MARK transition
                  if (bit) { // transition detected
                     state = ASYNC_CHECK_IDLE;
                     word_pos=0;
                  }
                  break;
               case ASYNC_CHECK_IDLE:    // Check idle for half a bit
                  word_pos++;
                  if (bit) { // Mark
                     if (word_pos>=(bit_len/2)) { // idle verified
                        state = ASYNC_IDLE;
                     }
                  } else { // Noise detection on idle
                     state = ASYNC_WAIT_IDLE;
                  }
                  break;
            }
            in_count++;
         }

         consume_each (in_count);

         return (out_count);
      }
   }
}
