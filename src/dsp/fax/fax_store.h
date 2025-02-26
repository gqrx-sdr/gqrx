/*
 * fax decoded lines sink block header
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

#ifndef FAX_STORE_H
#define FAX_STORE_H

#include <gnuradio/sync_block.h>
#include <string>
#include <mutex>
#include <queue>

namespace gr {
   namespace fax {
      class fax_store : public gr::sync_block {
         public:

#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<fax_store> sptr;
#else
            typedef std::shared_ptr<fax_store> sptr;
#endif

            static sptr make(int len);

            int get_data(unsigned char *out,unsigned int *len);
            void reset();

            int work (int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items);

         private:
            fax_store(int len);
            ~fax_store();

            std::mutex d_mutex;

	    std::queue<std::vector<unsigned char>> d_data;
            std::vector<unsigned char> current_line;
      };

   }
}
#endif // FAX_STORE_H
