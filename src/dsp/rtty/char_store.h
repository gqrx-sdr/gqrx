/*
 * character sink with baudot decoder block header
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

#ifndef CHAR_STORE_H
#define CHAR_STORE_H

#include <gnuradio/sync_block.h>
#include <string>
#include <mutex>
#include <queue>

class char_store : public gr::sync_block {
   public:

#if GNURADIO_VERSION < 0x030900
      typedef boost::shared_ptr<char_store> sptr;
#else
      typedef std::shared_ptr<char_store> sptr;
#endif

      static sptr make(int size,bool baudot);

      int get_data(std::string &out);

      void set_baudot(bool baudot);

      int work (int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items);

   private:
      char_store(int size,bool baudot);
      ~char_store();

      void store(std::string data);
      std::mutex d_mutex;
      std::queue<std::string> d_data;

      bool d_baudot;
      bool d_figures;
      int last_char;
      bool esc;
};

#endif // CHAR_STORE_H
