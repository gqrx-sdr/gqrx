/*
 * asynchronous receiver block implementation header
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

#ifndef ASYNC_RX_IMPL_H
#define ASYNC_RX_IMPL_H

#include "async_rx.h"
#include <mutex>

namespace gr {
   namespace rtty {

      class async_rx_impl : public async_rx
      {
         public:
            async_rx_impl(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity);
            ~async_rx_impl();

            void forecast(int noutput_items, gr_vector_int &ninput_items_required);

            int general_work(int noutput_items,
                  gr_vector_int &ninput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items);

            void set_sample_rate(float sample_rate);
            float sample_rate() const ;

            void set_bit_rate(float bit_rate);
            float bit_rate() const ;

            void set_word_len(char word_len);
            char word_len() const ;

            void set_parity(enum async_rx_parity parity);
            enum async_rx_parity parity() const ;

            void reset();

         private:
            enum async_rx_impl_state {
               ASYNC_WAIT_IDLE = 0,
               ASYNC_CHECK_IDLE,
               ASYNC_IDLE,
               ASYNC_CHECK_START,
               ASYNC_GET_BIT,
               ASYNC_CHECK_PARITY,
               ASYNC_CHECK_STOP
            };

            float d_sample_rate;
            float d_bit_rate;
            unsigned char d_word_len;
            enum async_rx_parity d_parity;

            std::mutex d_mutex;

            enum async_rx_impl_state state;
            float bit_len;
            unsigned int word_pos; // number of sample from start transition;
            unsigned char bit_pos;
            unsigned char bit_count; // number of mark bit (for parity)
            unsigned char word; // recived word

            bool cd;
            bool bit;
            bool send_esc;
            bool send_sot;
            bool send_eot;
      };
   }
}

#endif // ASYNC_RX_H
