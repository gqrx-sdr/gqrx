/*
 * character sink with baudot decoder block implementation
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
#include "char_store.h"
#include "async_rx.h"

char_store::sptr char_store::make(int size,bool baudot)
{
   return gnuradio::get_initial_sptr(new char_store(size,baudot));
}

char_store::char_store(int size,bool baudot) : gr::sync_block ("char_store",
      gr::io_signature::make (1,1, sizeof(char)),
      gr::io_signature::make (0, 0, 0)),
   d_baudot(baudot),
   d_figures(false),
   last_char(0) {
      esc= false;
   }

char_store::~char_store () {
}

void char_store::store(std::string data) {
   std::lock_guard<std::mutex> lock(d_mutex);
   d_data.push(data);
}

void char_store::set_baudot(bool baudot) {
   d_baudot = baudot;
   d_figures = false;
   esc = false;
}

#define BAUDOT_LETTERS 31
#define BAUDOT_FIGURES 27

static const char Baudot_letters[][32] = {
   "\000E\nA SIU\rDRJNFCKTZLWHYPQOBG\000MXV",
};

static const char Baudot_figures[][32] = {
   "\0003\n- \a87\r$4',!:(5\")2#6019?&\000./;",
};

int char_store::work (int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items) {
   std::string data;
   int i;
   unsigned int c;

   for (i=0;i<noutput_items;i++) {
      c = ((char*)input_items[0])[i];

      if (esc) {
         esc = false;
         if (c != ASYNC_RX_ESC_CHAR) {
            switch (c) {
               case ASYNC_RX_SOT_CHAR: // Do noting when transmission restart
                  fprintf(stderr,"\nSOT\n");
                  continue;
               case ASYNC_RX_EOT_CHAR:
                  fprintf(stderr,"\nEOT\n");
                  if (!data.empty()) {
                     store(data);
                     data.clear();
                  }
                  // send empty line on end of transmission
                  store(data);
                  continue;
            }
         }
      } else if (c == ASYNC_RX_ESC_CHAR) {
         esc = true;
         continue;
      }

      if (d_baudot) {
         c &= 0x1f;
         if (d_figures) {
            if (c == BAUDOT_LETTERS) {
               d_figures = false;
               continue;
            }
            else {
               c = Baudot_figures[0][c];
            }
         }
         else {
            if (c == BAUDOT_FIGURES) {
               d_figures = true;
               continue;
            }
            else {
               c = Baudot_letters[0][c];
            }
         }
      }

      if (c) {
         if (c=='\n' || c=='\r') {
            if (last_char != '\n' && last_char!= '\r')
               data += '\n';
         } else
            data += c;
         last_char = c;
         fputc(c,stderr);
      }
   }

   if (!data.empty())
      store(data);

   return noutput_items;
}

int char_store::get_data (std::string &data) {

   std::lock_guard<std::mutex> lock(d_mutex);

   if (!d_data.empty()) {
      data=d_data.front();
      d_data.pop();

      return d_data.size();
   }

   return -1;
}
