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
#include <thread>
#include <queue>
#include <condition_variable>
		

/*!
    * \brief Write stream to file without blocking.
    * \ingroup file_operators_blk
    */
class BLOCKS_API file_sink : virtual public gr::sync_block
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<file_sink> sptr;
#else
    typedef std::shared_ptr<file_sink> sptr;
#endif
    typedef struct {
        int len;
        int size;
        char * data;
        } s_data;

    /*!
    * \brief Make a file sink.
    * \param itemsize size of the input data items.
    * \param filename name of the file to open and write output to.
    * \param append if true, data is appended to the file instead of
    *        overwriting the initial content.
    */
    static sptr make(size_t itemsize, const char *filename, int sample_rate, bool append=false, int buffers_max=8);
    private:
      size_t d_itemsize;

    protected:
      FILE        *d_fp;        // current FILE pointer
      FILE        *d_new_fp;    // new FILE pointer
      bool         d_updated;   // is there a new FILE pointer?
      bool         d_is_binary;
      std::mutex d_mutex;
      bool         d_unbuffered;
      bool         d_append;
      std::queue<s_data> d_queue;
      std::queue<s_data> d_free;
      std::condition_variable d_writer_trigger;
      std::condition_variable d_writer_ready;
      bool         d_writer_finish;
      std::thread * d_writer_thread;
      s_data       d_sd;
      int          d_sd_max;
      int          d_buffers_used;
      int          d_buffers_max;
      bool         d_failed;
      bool         d_closing;
      size_t       d_written;

    private:
    void writer();

    public:
      file_sink(size_t itemsize, const char *filename, int sample_rate, bool append, int buffers_max=8);
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
       * \brief turn on unbuffered writes for slower outputs
       */
      void set_unbuffered(bool unbuffered);

      int  get_buffer_usage();
      int  get_buffers_max();
      bool get_failed();
      size_t get_written();
      void set_buffers_max(int buffers_max);

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
};


#endif
