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
        file_sink::s_data item;
        int written=0;
        char * p;
        std::cout << "Writer start" <<std::endl;
        while(true)
        {
            while(queue.pop(item))
            {
                written=0;
                p=item.data;
                while(written<item.len)
                {
                    gr::thread::scoped_lock guard(d_mutex);
                    if(!d_fp)
                    {
                        delete [] item.data;
                        return;
                    }
                    int count = fwrite(p, 1, item.len-written, d_fp);
                    if(count == 0)
                    {
                        if(ferror(d_fp))
                        {
//                            std::stringstream s;
                            std::cerr << "file_sink write failed with error " << fileno(d_fp) << std::endl;
//                            throw std::runtime_error(s.str());
                            return;
                        }
                        else // is EOF
                            break;
                    }else{
                        boost::unique_lock<boost::mutex> lock(writer_mutex);
                        buffers_used--;
                    }
                    written+=count;
                    p+=count;
                }
                delete [] item.data;
                fflush (d_fp);
            }
            if(writer_finish)
            {
                std::cout << "Writer finished" <<std::endl;
                return;
            }
            else
            {
               writer_ready.notify_one();
               boost::unique_lock<boost::mutex> lock(writer_mutex);
               writer_trigger.wait(lock);
            }
        }
    }

    file_sink::sptr file_sink::make(size_t itemsize, const char *filename, bool append)
    {
        return gnuradio::get_initial_sptr
            (new file_sink(itemsize, filename, append));
    }


    file_sink::file_sink(size_t itemsize, const char *filename, bool append)
      : sync_block("file_sink",
                      gr::io_signature::make(1, 1, itemsize),
                      gr::io_signature::make(0, 0, 0)),
                      d_itemsize(itemsize),
                      d_fp(0), d_new_fp(0), d_updated(false), d_is_binary(true),
                      d_append(append), queue(512), writer_finish(false)
    {
        if (!open(filename))
            throw std::runtime_error ("can't open file");
        writer_thread=new boost::thread(boost::bind(&file_sink::writer,this));
        sd.data=NULL;
        sd.len=0;
        buffers_used=0;
    }

    file_sink::~file_sink()
    {
        
        if(sd.len>0)
        {
            queue.push(sd);
            sd.len=0;
            sd.data=NULL;
        }
        close();
        writer_finish=true;
        writer_trigger.notify_one();
        writer_thread->join();
        delete writer_thread;
        if(d_fp)
        {
            fclose(d_fp);
            d_fp = 0;
        }
    }

    bool file_sink::open(const char *filename)
    {
        gr::thread::scoped_lock guard(d_mutex);	// hold mutex for duration of this function

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

        d_updated = true;
        return d_new_fp != 0;
    }

    void file_sink::close()
    {
        gr::thread::scoped_lock guard(d_mutex);	// hold mutex for duration of this function
        if(sd.len>0)
        {
            queue.push(sd);
            sd.len=0;
            sd.data=NULL;
        }
        writer_trigger.notify_one();
        writer_ready.wait(guard);
        if(d_new_fp)
        {
            fclose(d_new_fp);
            d_new_fp = 0;
        }
        d_updated = true;
    }

    void file_sink::do_update()
    {
        if(d_updated)
        {
            gr::thread::scoped_lock guard(d_mutex);   // hold mutex for duration of this block
            if(d_fp)
                fclose(d_fp);
            d_fp = d_new_fp;                     // install new file pointer
            d_new_fp = 0;
            d_updated = false;
        }
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
        do_update();                    // update d_fp is reqd
        if(sd.data==NULL)
            sd.data=new char [sd_max];
        if(sd.len+len_bytes>sd_max)
        {
            int l_buffers_used;
            queue.push(sd);
            sd.data=new char [sd_max];
            sd.len=0;
            writer_trigger.notify_one();
            {
                boost::unique_lock<boost::mutex> lock(writer_mutex);
                l_buffers_used=++buffers_used;
            }
            std::cerr << "buffers="<<l_buffers_used<<std::endl;
        }
        memcpy(&sd.data[sd.len],inbuf,len_bytes);
        sd.len+=len_bytes;
        return noutput_items;
#if 0
        int  nwritten = 0;
        static int n_output_min=0x7fffffff;
        static int n_output_max=0;
        static int cnt=0;

        do_update();                    // update d_fp is reqd

        if(!d_fp)
            return noutput_items;         // drop output on the floor

        while(nwritten < noutput_items)
        {
            int count = fwrite(inbuf, d_itemsize, noutput_items - nwritten, d_fp);
            if(count == 0)
            {
            if(ferror(d_fp))
            {
                std::stringstream s;
                s << "file_sink write failed with error " << fileno(d_fp) << std::endl;
                throw std::runtime_error(s.str());
            }
            else // is EOF
            {
                break;
            }
            }
            nwritten += count;
            inbuf += count * d_itemsize;
        }
        if(nwritten>n_output_max)
            n_output_max=nwritten;
        if(nwritten>n_output_min)
            n_output_min=nwritten;
        if(cnt>=100)
        {
            cnt=0;
            std::cout<<"min="<<n_output_min<<" max="<<n_output_max<<std::endl;
        }
        else
            cnt++;

        if(d_unbuffered)
            fflush (d_fp);

        return nwritten;
#endif
    }

    int file_sink::get_buffer_usage()
    {
        return buffers_used;
    }
