/*
 * fax decoder block header
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

#ifndef FAX_DECODER_H
#define FAX_DECODER_H

#include <gnuradio/block.h>

namespace gr {
   namespace fax {
      class fax_decoder : virtual public gr::block
      {
         public:

#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<fax_decoder> sptr;
#else
            typedef std::shared_ptr<fax_decoder> sptr;
#endif

            static sptr make(float sample_rate, float lpm, float ioc);

            virtual void set_sample_rate(float sample_rate) = 0;
            virtual float sample_rate() const = 0;

            virtual void set_lpm(float lpm) = 0;
            virtual float lpm() const = 0;

            virtual void set_ioc(float ioc) = 0;
            virtual float ioc() const = 0;

            virtual void set_state(int state) = 0;
            virtual int state() const = 0;

            virtual void reset() = 0;
      };

   } // namespace fax
} // namespace gr

#endif // FAX_DECODER_H
