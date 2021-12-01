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

#include <interfaces/file_sink.h>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include <stdio.h>
#include <gnuradio/thread/thread.h>
#include <gnuradio/io_signature.h>
#include <stdexcept>

// win32 (mingw/msvc) specific
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef O_BINARY
#define	OUR_O_BINARY O_BINARY
#else
#define	OUR_O_BINARY 0
#endif

// should be handled via configure
#ifdef O_LARGEFILE
#define	OUR_O_LARGEFILE	O_LARGEFILE
#else
#define	OUR_O_LARGEFILE 0
#endif

    void file_sink::writer()
    {
        s_data item;
        int written=0;
        char * p;
        FILE * old_fp=NULL;
        int count = 0;
        while(true)
        {
            std::unique_lock<std::mutex> guard(d_mutex);   // hold mutex for duration of this block
            while(!d_queue.empty())
            {
                item=d_queue.front();
                d_queue.pop();
                written=0;
                p=item.data;
                while(written<item.len)
                {
                    if(d_updated)
                    {
                        old_fp=d_fp;
                        d_fp = d_new_fp;                     // install new file pointer
                        d_new_fp = 0;
                        d_updated = false;
                    }
                    if(d_fp && !d_failed)
                    {
                        guard.unlock();
                        count = fwrite(p, 1, item.len-written, d_fp);
                        guard.lock();
                        if(count == 0)
                        {
                            if(ferror(d_fp))
                            {
                                std::cerr << "file_sink write failed with error " << fileno(d_fp) << std::endl;
                                d_failed=true;
                                break;
                            }
                            else // is EOF
                                break;
                        }
                    }else
                        break;
                    written+=count;
                    p+=count;
                }
                guard.unlock();
                delete [] item.data;
                if(d_fp && !d_failed)
                    fflush (d_fp);
                if(old_fp)
                {
                    fclose(old_fp);
                    old_fp=NULL;
                }
                guard.lock();
                d_buffers_used--;
                d_written += written;
            }
            if(d_writer_finish)
            {
                return;
            }
            else
            {
               d_writer_ready.notify_one();
               d_writer_trigger.wait(guard);
            }
        }
    }

    file_sink::sptr file_sink::make(size_t itemsize, const char *filename, int sample_rate, bool append, int buffers_max)
    {
        return gnuradio::get_initial_sptr
            (new file_sink(itemsize, filename, sample_rate, append, buffers_max));
    }


    file_sink::file_sink(size_t itemsize, const char *filename, int sample_rate, bool append, int buffers_max)
      : sync_block("file_sink",
                      gr::io_signature::make(1, 1, itemsize),
                      gr::io_signature::make(0, 0, 0)),
                      d_itemsize(itemsize),
                      d_fp(0), d_new_fp(0), d_updated(false), d_is_binary(true),
                      d_append(append), d_writer_finish(false),
                      d_sd_max(std::max(8192,sample_rate)*itemsize), d_buffers_used(0), d_buffers_max(buffers_max)
    {
        if (!open(filename))
            throw std::runtime_error ("can't open file");
        d_writer_thread=new std::thread(std::bind(&file_sink::writer,this));
        d_sd.data=NULL;
        d_sd.len=0;
        d_buffers_used=0;
        d_closing=false;
    }

    file_sink::~file_sink()
    {
        d_closing=true;
        if(d_sd.len>0)
        {
            d_queue.push(d_sd);
            d_sd.len=0;
            d_sd.data=NULL;
        }
        close();
        d_writer_finish=true;
        d_writer_trigger.notify_one();
        d_writer_thread->join();
        delete d_writer_thread;
        if(d_fp)
        {
            fclose(d_fp);
            d_fp = 0;
        }
    }

    bool file_sink::open(const char *filename)
    {

        if(d_updated)
            return false;
        // we use the open system call to get access to the O_LARGEFILE flag.
        int fd;
        int flags;
        if(d_append)
        {
            flags = O_WRONLY|O_CREAT|O_APPEND|OUR_O_LARGEFILE|OUR_O_BINARY;
        }
        else
        {
            flags = O_WRONLY|O_CREAT|O_TRUNC|OUR_O_LARGEFILE|OUR_O_BINARY;
        }
        if((fd = ::open(filename, flags, 0664)) < 0)
        {
            perror(filename);
            return false;
        }
        if(d_new_fp)          // if we've already got a new one open, close it
        {
            fclose(d_new_fp);
            d_new_fp = 0;
        }

        if((d_new_fp = fdopen (fd, d_is_binary ? "wb" : "w")) == NULL)
        {
            perror (filename);
            ::close(fd);        // don't leak file descriptor if fdopen fails.
        }

        {
            std::unique_lock<std::mutex> guard(d_mutex);
            d_updated = true;
            d_failed = false;
            d_closing = false;
            d_written = 0;
        }
        return d_new_fp != 0;
    }

    void file_sink::close()
    {
        std::unique_lock<std::mutex> guard(d_mutex);
        //prevent new buffers submission
        d_closing=true;
        //submit last buffer
        if(d_sd.len>0)
        {
            d_queue.push(d_sd);
            d_sd.data=NULL;
            d_sd.len=0;
        }
        //wake the thread
        d_writer_trigger.notify_one();
        //wait for thread to finish writeng buffers
        d_writer_ready.wait(guard);
        if(d_new_fp)
        {
            fclose(d_new_fp);
            d_new_fp = 0;
        }
        d_updated = true;
    }

    void file_sink::set_unbuffered(bool unbuffered)
    {
        d_unbuffered = unbuffered;
    }


    int file_sink::work(int noutput_items,
                         gr_vector_const_void_star &input_items,
                         gr_vector_void_star &output_items)
    {
        char *inbuf = (char*)input_items[0];
        int len_bytes=noutput_items*d_itemsize;
        //do not queue more buffers if we are closing the file
        std::unique_lock<std::mutex> guard(d_mutex);
        if(d_closing||d_failed)
            return noutput_items;
        if(d_sd.data==NULL)
        {
            if(len_bytes>d_sd_max)
                d_sd_max=len_bytes;
            d_sd.data=new char [d_sd_max];
        }
        if(d_sd.len+len_bytes>d_sd_max)
        {
            if(d_sd.len==0)
            {
                free(d_sd.data);
                d_sd.data=NULL;
            }else{
                d_queue.push(d_sd);
                d_writer_trigger.notify_one();
            }
            if(len_bytes>d_sd_max)
                d_sd_max=len_bytes;
            d_sd.data=new char [d_sd_max];
            d_sd.len=0;
            ++d_buffers_used;
            if(d_buffers_used>d_buffers_max)
            {
                d_failed=true;
            }
        }
        memcpy(&d_sd.data[d_sd.len],inbuf,len_bytes);
        d_sd.len+=len_bytes;
        return noutput_items;
    }

    int file_sink::get_buffer_usage()
    {
        return d_buffers_used;
    }

    int file_sink::get_buffers_max()
    {
        return d_buffers_max;
    }

    bool file_sink::get_failed()
    {
        return d_failed;
    }

    size_t file_sink::get_written()
    {
        return d_written;
    }

    void file_sink::set_buffers_max(int buffers_max)
    {
        //At least one buffer should be present
        std::unique_lock<std::mutex> guard(d_mutex);
        if(buffers_max<=0)
            buffers_max=1;
        d_buffers_max=buffers_max;
    }
