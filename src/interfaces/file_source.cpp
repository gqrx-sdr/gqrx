/* -*- c++ -*- */
/*
 * Copyright 2012, 2018 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


#include "file_source.h"
#include <gnuradio/io_signature.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <stdexcept>

#ifdef _MSC_VER
#define GR_FSEEK _fseeki64
#define GR_FTELL _ftelli64
#define GR_FSTAT _fstat
#define GR_FILENO _fileno
#define GR_STAT _stat
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#else
#define GR_FSEEK fseeko
#define GR_FTELL ftello
#define GR_FSTAT fstat
#define GR_FILENO fileno
#define GR_STAT stat
#endif

#define READ_MAX (16*1024*1024)
#define NO_WAIT_START

void file_source::reader()
{
    uint8_t * last=&d_buf.data()[d_buf.size()];
    uint8_t * p;
    FILE * old_fp = NULL;
    int count = 0;
    while (true)
    {
        std::unique_lock<std::mutex> guard(d_mutex);   // hold mutex for duration of this block
        do
        {
            count = 0;
            p=d_wp;
            if (d_updated)
            {
                old_fp = d_fp;
                d_fp = d_new_fp;                     // install new file pointer
                d_new_fp = 0;
                d_updated = false;
                d_file_begin = true;
                d_eof = false;
            }
            if(d_seek)
            {
                guard.unlock();
                d_seek_ok = (GR_FSEEK((FILE*)d_fp, d_seek_point * d_itemsize, SEEK_SET) == 0);
                guard.lock();
                d_seek = false;
                d_buffering = true;
                p=d_rp=d_wp=d_buf.data();
                d_items_remaining = d_length_items + d_start_offset_items - d_seek_point;
                d_eof = false;
                continue;
            }
            if(d_eof)
            {
                break;
            }
            if (d_fp && !d_failed)
            {
                if (p == last)
                {
                    if (d_rp == d_buf.data())
                        break;
                    else
                        p = d_buf.data();
                }
                int64_t read_bytes = (last - p);
                if (d_rp > p)
                    read_bytes = d_rp - d_itemsize - p;
                if (read_bytes <= 0)
                    break;
                if (read_bytes > READ_MAX)
                    read_bytes = READ_MAX;
                guard.unlock();
                count = fread(p, d_itemsize, read_bytes / d_itemsize, d_fp) * d_itemsize;
                guard.lock();
                if (count == 0)
                {
                    if (ferror(d_fp))
                    {
                        std::cerr << "file_source read failed with error " << fileno(d_fp) << std::endl;
                        d_failed = true;
                    }
                    else // is EOF
                    {
                        if (d_repeat && d_seekable)
                        {
                            guard.unlock();
                            d_seek_ok = (GR_FSEEK((FILE*)d_fp, d_start_offset_items * d_itemsize, SEEK_SET) == 0);
                            guard.lock();
                        }
                        else
                            d_eof = true;
                    }
                    break;
                }
            }
            else
                break;
            p += count;
            d_wp = p;
        }
            while (count);
        if (old_fp)
        {
            guard.unlock();
            fclose(old_fp);
            guard.lock();
            old_fp = NULL;
        }
        if (d_reader_finish)
        {
            return;
        }
        else
        {
            d_reader_ready.notify_one();
            d_buffering = false;
            d_reader_wake.wait(guard);
        }
    }
}

file_source::sptr file_source::make(size_t itemsize,
                                    const char* filename,
                                    uint64_t start_offset_items,
                                    uint64_t length_items, int sample_rate,
                                    bool repeat, int buffers_max)
{
    return gnuradio::get_initial_sptr(new file_source(
        itemsize, filename, start_offset_items, length_items,
        sample_rate, repeat, buffers_max));
}

file_source::file_source(size_t itemsize,
                                   const char* filename,
                                   uint64_t start_offset_items,
                                   uint64_t length_items, int sample_rate,
                                   bool repeat, int buffers_max)
    : sync_block(
          "file_source", gr::io_signature::make(0, 0, 0), gr::io_signature::make(1, 1, itemsize)),
      d_itemsize(itemsize),
      d_start_offset_items(start_offset_items),
      d_length_items(length_items),
      d_fp(0),
      d_new_fp(0),
      d_repeat(repeat),
      d_updated(false),
      d_file_begin(true),
      d_repeat_cnt(0),
      d_add_begin_tag(pmt::PMT_NIL),
      d_reader_finish(false)
{
    open(filename, repeat, start_offset_items, length_items);

    std::stringstream str;
    str << name() << unique_id();
    _id = pmt::string_to_symbol(str.str());
    d_eof = false;
    d_closing = false;
    d_failed = false;
    d_seek = false;
    d_buffering = false;
    if(buffers_max <= 0)
        buffers_max = 1;
    d_buffer_size = buffers_max * std::max(8192, sample_rate) * itemsize;
    d_buf.resize(d_buffer_size);
    d_wp = d_rp = d_buf.data();
    {
        std::unique_lock<std::mutex> guard(d_mutex);
        d_reader_thread = new std::thread(std::bind(&file_source::reader, this));
        d_buffering = true;
        d_reader_wake.notify_one();
#ifdef WAIT_START
        d_reader_ready.wait(guard);
#endif
    }
}

file_source::~file_source()
{
    d_reader_finish = true;
    d_reader_wake.notify_one();
    d_reader_thread->join();
    delete d_reader_thread;
    if (d_fp)
        fclose((FILE*)d_fp);
    if (d_new_fp)
        fclose((FILE*)d_new_fp);
}

bool file_source::seek(int64_t seek_point, int whence)
{
    if (d_seekable)
    {
        std::unique_lock<std::mutex> guard(d_mutex);

        d_seek_point = seek_point + d_start_offset_items;
        switch (whence)
        {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            d_seek_point += (d_length_items - d_items_remaining);
            break;
        case SEEK_END:
            d_seek_point = d_length_items - seek_point;
            break;
        default:
            GR_LOG_WARN(d_logger, "bad seek mode");
            return 0;
        }

        if ((seek_point < (int64_t)d_start_offset_items) ||
            (seek_point > (int64_t)(d_start_offset_items + d_length_items - 1)))
        {
            GR_LOG_WARN(d_logger, "bad seek point");
            return 0;
        }
        d_seek = true;
        d_seek_ok = 0;
        d_reader_wake.notify_one();
#ifdef WAIT_START
        d_reader_ready.wait(guard);
#endif
        return d_seek_ok;
    }
    else
    {
        GR_LOG_WARN(d_logger, "file not seekable");
        return 0;
    }
}


void file_source::open(const char* filename,
                       bool repeat,
                       uint64_t start_offset_items,
                       uint64_t length_items)
{
    // obtain exclusive access for duration of this function
    std::unique_lock<std::mutex> guard(d_mutex);

    if (d_new_fp)
    {
        fclose(d_new_fp);
        d_new_fp = 0;
    }

    if ((d_new_fp = fopen(filename, "rb")) == NULL)
    {
        GR_LOG_ERROR(d_logger, std::string(filename) + ": " + std::string(strerror(errno)));
        throw std::runtime_error("can't open file");
    }

    struct GR_STAT st;

    if (GR_FSTAT(GR_FILENO(d_new_fp), &st))
    {
        GR_LOG_ERROR(d_logger, std::string(filename) + ": " + std::string(strerror(errno)));
        throw std::runtime_error("can't fstat file");
    }
    d_seekable = (S_ISREG(st.st_mode));

    uint64_t file_size;

    if (d_seekable)
    {
        // Check to ensure the file will be consumed according to item size
        GR_FSEEK(d_new_fp, 0, SEEK_END);
        file_size = GR_FTELL(d_new_fp);

        // Make sure there will be at least one item available
        if ((file_size / d_itemsize) < (start_offset_items + 1))
        {
            if (start_offset_items)
            {
                GR_LOG_WARN(d_logger, "file is too small for start offset");
            }
            else
            {
                GR_LOG_WARN(d_logger, "file is too small");
            }
            fclose(d_new_fp);
            throw std::runtime_error("file is too small");
        }
    }
    else
    {
        file_size = INT64_MAX;
    }

    uint64_t items_available = (file_size / d_itemsize - start_offset_items);

    // If length is not specified, use the remainder of the file. Check alignment at end.
    if (length_items == 0)
    {
        length_items = items_available;
        if (file_size % d_itemsize)
        {
            GR_LOG_WARN(d_logger, "file size is not a multiple of item size");
        }
    }

    // Check specified length. Warn and use available items instead of throwing an
    // exception.
    if (length_items > items_available)
    {
        length_items = items_available;
        GR_LOG_WARN(d_logger, "file too short, will read fewer than requested items");
    }

    // Rewind to start offset
    if (d_seekable)
    {
        GR_FSEEK(d_new_fp, start_offset_items * d_itemsize, SEEK_SET);
    }

    d_updated = true;
    d_repeat = repeat;
    d_start_offset_items = start_offset_items;
    d_length_items = length_items;
    d_items_remaining = length_items;
}

void file_source::close()
{
    // obtain exclusive access for duration of this function
    std::unique_lock<std::mutex> guard(d_mutex);

    if (d_new_fp != NULL)
    {
        fclose(d_new_fp);
        d_new_fp = NULL;
    }
    d_updated = true;
}

void file_source::set_begin_tag(pmt::pmt_t val) { d_add_begin_tag = val; }

int file_source::work(int noutput_items,
                           gr_vector_const_void_star& input_items,
                           gr_vector_void_star& output_items)
{
    char* o = (char*)output_items[0];
    uint64_t size = noutput_items;
#if 0
    do_update(); // update d_fp is reqd
#endif
    if (d_fp == NULL)
        throw std::runtime_error("work with file not open");

    std::unique_lock<std::mutex> guard(d_mutex);

    // No items remaining - all done
    if (d_items_remaining == 0)
        return WORK_DONE;

    while (size)
    {
        d_reader_wake.notify_one();
        if(d_wp == d_rp)
            d_buffering = true;
        if(d_buffering)
        {
            //d_reader_ready.wait(guard);
            //output zeroes while we are buffering
            guard.unlock();
            memset(o, 0, d_itemsize * noutput_items);
            return noutput_items;
        }
        uint64_t bytes_avail = (d_wp >= d_rp) ? (d_wp - d_rp):
                               (&d_buf.data()[d_buf.size()] - d_rp);
        if(bytes_avail >= d_itemsize)
        {
            uint64_t items_to_copy = bytes_avail / d_itemsize;
            uint64_t to_copy = std::min(size, items_to_copy);
            if(to_copy > d_items_remaining)
                to_copy = d_items_remaining;
            guard.unlock();
            if (d_file_begin && d_add_begin_tag != pmt::PMT_NIL)
            {
                add_item_tag(0,
                            nitems_written(0) + noutput_items - size,
                            d_add_begin_tag,
                            pmt::from_long(d_repeat_cnt),
                            _id);
                d_file_begin = false;
            }
            memcpy(o, d_rp, to_copy * d_itemsize);
            guard.lock();
            o += to_copy * d_itemsize;
            d_rp += to_copy * d_itemsize;
            if(d_rp == &d_buf.data()[d_buf.size()])
                d_rp = d_buf.data();
            size -= to_copy;
            d_items_remaining -= to_copy;
            if (d_items_remaining == 0)
            {

                // Repeat: rewind and request tag
                if (d_repeat && d_seekable)
                {
                    if (d_add_begin_tag != pmt::PMT_NIL)
                    {
                        add_item_tag(0,
                                    nitems_written(0) + noutput_items - size,
                                    d_add_begin_tag,
                                    pmt::from_long(d_repeat_cnt),
                                    _id);
                        d_file_begin = false;
                    }
                    d_items_remaining = d_length_items - d_start_offset_items;
                }
                else
                    break;
            }
        }
        else
            d_buffering = true;
#if 0
        // Add stream tag whenever the file starts again

        uint64_t nitems_to_read = std::min(size, d_items_remaining);

        // Since the bounds of the file are known, unexpected nitems is an error
        if (nitems_to_read != fread(o, d_itemsize, nitems_to_read, (FILE*)d_fp))
            throw std::runtime_error("fread error");

        size -= nitems_to_read;
        d_items_remaining -= nitems_to_read;
        o += nitems_to_read * d_itemsize;

        // Ran out of items ("EOF")
#endif
    }
    return (noutput_items - size);
}

uint64_t file_source::tell()
{
    std::unique_lock<std::mutex> guard(d_mutex);
    return (d_length_items - d_items_remaining) * d_itemsize;
}

int file_source::get_buffer_usage()
{
    std::unique_lock<std::mutex> guard(d_mutex);
    return (d_wp >= d_rp ? d_wp - d_rp :
           d_wp + d_buffer_size - d_rp) * 100llu / d_buffer_size;
}
