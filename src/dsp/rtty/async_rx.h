/*
 * asynchronous receiver block header
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

#ifndef ASYNC_RX_H
#define ASYNC_RX_H

#include <gnuradio/block.h>

#define ASYNC_RX_ESC_CHAR	(0x1b)
#define ASYNC_RX_SOT_CHAR	(0x02) // STX
#define ASYNC_RX_EOT_CHAR	(0x03) // ETX

namespace gr {
   namespace rtty {
      class async_rx : virtual public gr::block
      {
         public:

            enum async_rx_parity {
               ASYNC_RX_PARITY_NONE = 0,
               ASYNC_RX_PARITY_ODD,
               ASYNC_RX_PARITY_EVEN,
               ASYNC_RX_PARITY_MARK,
               ASYNC_RX_PARITY_SPACE,
               ASYNC_RX_PARITY_DONTCARE
            };

#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<async_rx> sptr;
#else
            typedef std::shared_ptr<async_rx> sptr;
#endif

            static sptr make(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity);

            virtual void set_sample_rate(float sample_rate) = 0;
            virtual float sample_rate() const = 0;

            virtual void set_bit_rate(float bit_rate) = 0;
            virtual float bit_rate() const = 0;

            virtual void set_word_len(char bits_per_word) = 0 ;
            virtual char word_len() const = 0;

            virtual void set_parity(enum async_rx_parity parity) = 0;
            virtual enum async_rx_parity parity() const = 0;

            virtual void reset() = 0;
      };

   } // namespace rtty
} // namespace gr

#endif // ASYNC_RX_H
