/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2021 vladisslav2011@gmail.com.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GQRX_FILE_SINK_C_H
#define GQRX_FILE_SINK_C_H

#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>
#include <boost/lockfree/queue.hpp>

/*!
    * \brief Write stream to file without blocking.
    * \ingroup file_operators_blk
    */
class BLOCKS_API file_sink : virtual public gr::sync_block
{
public:
    // file_sink::sptr
    typedef boost::shared_ptr<file_sink> sptr;
    typedef struct {
        int len;
        char * data;
        } s_data;

    /*!
    * \brief Make a file sink.
    * \param itemsize size of the input data items.
    * \param filename name of the file to open and write output to.
    * \param append if true, data is appended to the file instead of
    *        overwriting the initial content.
    */
    static sptr make(size_t itemsize, const char *filename, bool append=false);
    private:
      size_t d_itemsize;

    protected:
      FILE        *d_fp;        // current FILE pointer
      FILE        *d_new_fp;    // new FILE pointer
      bool         d_updated;   // is there a new FILE pointer?
      bool         d_is_binary;
      boost::mutex d_mutex;
      bool         d_unbuffered;
      bool         d_append;
      boost::mutex writer_mutex;
      boost::lockfree::queue<s_data> queue;
      boost::condition_variable writer_trigger;
      boost::condition_variable writer_ready;
      bool         writer_finish;
      boost::thread * writer_thread;
      s_data       sd;
      static const int sd_max=1024*1024*32;
      int          sd_p;
      int          buffers_used;

    private:
    void writer();

    public:
      file_sink(size_t itemsize, const char *filename, bool append);
      file_sink() {}
      ~file_sink();

      /*!
       * \brief Open filename and begin output to it.
       */
      bool open(const char *filename);

      /*!
       * \brief Close current output file.
       *
       * Closes current output file and ignores any output until
       * open is called to connect to another file.
       */
      void close();

      /*!
       * \brief if we've had an update, do it now.
       */
      void do_update();

      /*!
       * \brief turn on unbuffered writes for slower outputs
       */
      void set_unbuffered(bool unbuffered);

      int get_buffer_usage();

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
};


#endif