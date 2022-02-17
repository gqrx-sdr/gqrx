/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2008,2009,2013 Free Software Foundation, Inc.
 * Copyright 2022 vladisslav2011@gmail.com.
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

#include "wav_sink.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/thread/thread.h>
#include <cstring>
#include <stdexcept>
#include <QDateTime>
#include <QDir>

static const int SQL_REC_MIN_TIME = 10; /* Minimum squelch recorder time, seconds. */
static const int SQL_REC_MAX_GAP = 10; /* Maximum squelch recorder gap, seconds. */

wavfile_sink_gqrx::sptr wavfile_sink_gqrx::make(const char* filename,
                                      int n_channels,
                                      unsigned int sample_rate,
                                      wavfile_format_t format,
                                      wavfile_subformat_t subformat,
                                      bool append)
{
    return gnuradio::get_initial_sptr(new wavfile_sink_gqrx(
        filename, n_channels, sample_rate, format, subformat, append));
}

wavfile_sink_gqrx::wavfile_sink_gqrx(const char* filename,
                                     int n_channels,
                                     unsigned int sample_rate,
                                     wavfile_format_t format,
                                     wavfile_subformat_t subformat,
                                     bool append)
    : sync_block("wavfile_sink_gqrx",
                 gr::io_signature::make(1, n_channels, sizeof(float)),
                 gr::io_signature::make(0, 0, 0)),
      d_h{}, // Init with zeros
      d_append(append),
      d_fp(nullptr),
      d_new_fp(nullptr),
      d_updated(false),
      d_center_freq(0),
      d_offset(0),
      d_rec_dir(""),
      d_squelch_triggered(false),
      d_min_time_ms(0),
      d_max_gap_ms(0),
      d_min_time_samp(0),
      d_max_gap_samp(0),
      d_prev_action(ACT_NONE),
      d_prev_roffset(0)
{
    int bits_per_sample = 16;

    if (n_channels > s_max_channels)
        throw std::runtime_error("Number of channels greater than " +
                                 std::to_string(s_max_channels) + " not supported.");

    d_h.sample_rate = sample_rate;
    d_h.nchans = n_channels;
    d_h.format = format;
    d_h.subformat = subformat;
    switch (subformat) {
    case FORMAT_PCM_S8:
        bits_per_sample = 8;
        break;
    case FORMAT_PCM_16:
        bits_per_sample = 16;
        break;
    case FORMAT_PCM_24:
        bits_per_sample = 24;
        break;
    case FORMAT_PCM_32:
        bits_per_sample = 32;
        break;
    case FORMAT_PCM_U8:
        bits_per_sample = 8;
        break;
    case FORMAT_FLOAT:
        bits_per_sample = 32;
        break;
    case FORMAT_DOUBLE:
        bits_per_sample = 64;
        break;
    case FORMAT_VORBIS:
        bits_per_sample = 32;
        break;
    }
    set_bits_per_sample_unlocked(bits_per_sample);
    d_h.bytes_per_sample = d_bytes_per_sample_new;

    set_max_noutput_items(s_items_size);
    d_buffer.resize(s_items_size * d_h.nchans);

    if (filename)
        if (!open(filename))
            throw std::runtime_error("Can't open WAV file.");
    //FIXME Make this configurable?
    d_sob_key = pmt::intern("squelch_sob");
    d_eob_key = pmt::intern("squelch_eob");
    set_history(1 + sample_rate * (SQL_REC_MIN_TIME + SQL_REC_MAX_GAP));
}

void wavfile_sink_gqrx::set_center_freq(double center_freq)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_center_freq = center_freq;
}

void wavfile_sink_gqrx::set_offset(double offset)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_offset = offset;
}

void wavfile_sink_gqrx::set_rec_dir(std::string dir)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_rec_dir = dir;
}


bool wavfile_sink_gqrx::open(const char* filename)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    return open_unlocked(filename);
}

bool wavfile_sink_gqrx::open_unlocked(const char* filename)
{
    SF_INFO sfinfo;

    if (d_new_fp) { // if we've already got a new one open, close it
        sf_close(d_new_fp);
        d_new_fp = nullptr;
    }

    if (d_append) {
        // We are appending to an existing file, be extra careful here.
        sfinfo.format = 0;
        if (!(d_new_fp = sf_open(filename, SFM_RDWR, &sfinfo))) {
            std::cerr << "sf_open failed: " << filename << " " << strerror(errno) << std::endl;
            return false;
        }
        if (d_h.sample_rate != sfinfo.samplerate || d_h.nchans != sfinfo.channels ||
            d_h.format != (sfinfo.format & SF_FORMAT_TYPEMASK) ||
            d_h.subformat != (sfinfo.format & SF_FORMAT_SUBMASK)) {
            std::cerr << "Existing WAV file is incompatible with configured options."<<std::endl;
            sf_close(d_new_fp);
            return false;
        }
        if (sf_seek(d_new_fp, 0, SEEK_END) == -1) {
            std::cerr << "Seek error." << std::endl;
            return false; // This can only happen if the file disappears under our feet.
        }
    } else {
        memset(&sfinfo, 0, sizeof(sfinfo));
        sfinfo.samplerate = d_h.sample_rate;
        sfinfo.channels = d_h.nchans;
        switch (d_h.format) {
        case FORMAT_WAV:
            switch (d_h.subformat) {
            case FORMAT_PCM_U8:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_U8);
                break;
            case FORMAT_PCM_16:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
                break;
            case FORMAT_PCM_24:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);
                break;
            case FORMAT_PCM_32:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_32);
                break;
            case FORMAT_FLOAT:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);
                break;
            case FORMAT_DOUBLE:
                sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_DOUBLE);
                break;
            }
            break;
        case FORMAT_FLAC:
            switch (d_h.subformat) {
            case FORMAT_PCM_S8:
                sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_S8);
                break;
            case FORMAT_PCM_16:
                sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_16);
                break;
            case FORMAT_PCM_24:
                sfinfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_24);
                break;
            }
            break;
        case FORMAT_OGG:
            switch (d_h.subformat) {
            case FORMAT_VORBIS:
                sfinfo.format = (SF_FORMAT_OGG | SF_FORMAT_VORBIS);
                break;
            }
            break;
        case FORMAT_RF64:
            switch (d_h.subformat) {
            case FORMAT_PCM_U8:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_PCM_U8);
                break;
            case FORMAT_PCM_16:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_PCM_16);
                break;
            case FORMAT_PCM_24:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_PCM_24);
                break;
            case FORMAT_PCM_32:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_PCM_32);
                break;
            case FORMAT_FLOAT:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_FLOAT);
                break;
            case FORMAT_DOUBLE:
                sfinfo.format = (SF_FORMAT_RF64 | SF_FORMAT_DOUBLE);
                break;
            }
            break;
        }
        if (!(d_new_fp = sf_open(filename, SFM_WRITE, &sfinfo))) {
            std::cerr << "sf_open failed: " << filename << " "
                             << strerror(errno)<<std::endl;
            return false;
        }
    }
    d_updated = true;

    return true;
}

int wavfile_sink_gqrx::open_new()
{
    std::unique_lock<std::mutex> guard(d_mutex);
    return open_new_unlocked();
}

int wavfile_sink_gqrx::open_new_unlocked()
{
    // FIXME: option to use local time
    // use toUTC() function compatible with older versions of Qt.
    QString file_name = QDateTime::currentDateTime().toUTC().toString("gqrx_yyyyMMdd_hhmmss");
    QString filename = QString("%1/%2_%3.wav").arg(QString(d_rec_dir.data())).arg(file_name).arg(qint64(d_center_freq + d_offset));
    if (open_unlocked(filename.toStdString().data()))
    {
        if (d_rec_event)
            d_rec_event(d_filename = filename.toStdString(), true);
        return 0;
    }
    return 1;
}

void wavfile_sink_gqrx::close()
{
    std::unique_lock<std::mutex> guard(d_mutex);

    if (!d_fp)
        return;
    close_wav();
}

void wavfile_sink_gqrx::close_wav()
{
    sf_write_sync(d_fp);
    sf_close(d_fp);
    d_fp = nullptr;
    if (d_rec_event)
        d_rec_event(d_filename, false);
}

wavfile_sink_gqrx::~wavfile_sink_gqrx()
{
    set_rec_event_handler(nullptr);
    if (d_new_fp) {
        sf_close(d_new_fp);
        d_new_fp = nullptr;
    }
    close();

}

bool wavfile_sink_gqrx::stop()
{
    if (d_fp)
        sf_write_sync(d_fp);
    return true;
}

int wavfile_sink_gqrx::work(int noutput_items,
                            gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items)
{
    auto in = (float**)&input_items[0];
    int n_in_chans = input_items.size();
    int hist = history() - 1;
    int nwritten = hist;
    int writecount = noutput_items;
    std::vector<gr::tag_t> work_tags;
    std::unique_lock<std::mutex> guard(d_mutex); // hold mutex for duration of this block
    int roffset = 0; /** relative offset*/


    if (d_squelch_triggered)
    {
        uint64_t abs_N = nitems_read(0);
        get_tags_in_window(work_tags, 0, 0, noutput_items);
        for (const auto& tag : work_tags)
        {
            roffset = (tag.offset - abs_N);
            if (tag.key == d_sob_key)
            {
                if (d_prev_action == ACT_CLOSE)
                {
                    if (roffset + hist - d_prev_roffset <= d_max_gap_samp)
                    {
                        if (d_fp)
                        {
                            writeout(d_prev_roffset, roffset + hist - d_prev_roffset, n_in_chans, in);
                            nwritten = roffset + hist;
                            writecount = noutput_items - roffset;
                        }
                        d_prev_action = ACT_NONE;
                    }
                    else
                    {
                        if (d_fp)
                            close_wav();
                    }
                }
                d_prev_roffset = roffset + hist;
                if (!d_fp)
                    d_prev_action = ACT_OPEN;
            }
            if (tag.key == d_eob_key)
            {
                if (d_prev_action == ACT_OPEN)
                {
                    if (!d_fp && (roffset + hist - d_prev_roffset >= d_min_time_samp))
                    {
                        open_new_unlocked();
                        do_update();
                        if (d_fp)
                            writeout(d_prev_roffset, roffset + hist - d_prev_roffset, n_in_chans, in);
                    }
                }
                if (d_fp)
                    d_prev_action = ACT_CLOSE;
                else
                    d_prev_action = ACT_NONE;
                d_prev_roffset = roffset + hist;
            }
        }
    }
    switch(d_prev_action)
    {
    case ACT_NONE:
        do_update();                            // update: d_fp is read
        if (d_fp && writecount)
            writeout(nwritten, writecount, n_in_chans, in);
        break;
    case ACT_OPEN:
        if (hist - d_prev_roffset >= d_min_time_samp)
        {
            d_prev_action = ACT_NONE;
            if (!d_fp)
            {
                open_new_unlocked();
                do_update();
                if (d_fp)
                    writeout(d_prev_roffset, hist - d_prev_roffset + writecount, n_in_chans, in);
            }
        }
        break;
    case  ACT_CLOSE:
        if (hist - d_prev_roffset >= d_max_gap_samp)
        {
            if (d_fp)
            {
                close_wav();
            }
            d_prev_action = ACT_NONE;
        }
        break;
    }
    d_prev_roffset -= noutput_items;
    if (d_prev_roffset < 0)
        d_prev_roffset = 0;
    return noutput_items;
}

void wavfile_sink_gqrx::writeout(const int offset, const int writecount, const int n_in_chans, float** in)
{
    int nchans = d_h.nchans;
    int nwritten = 0;
    int bp = 0;
    int errnum;
    while(nwritten < writecount)
    {
        for (bp = 0; (nwritten < writecount) && (bp < s_items_size); nwritten++, bp++)
        {
            for (int chan = 0; chan < nchans; chan++)
            {
                // Write zeros to channels which are in the WAV file
                // but don't have any inputs here
                if (chan < n_in_chans)
                    d_buffer[chan + (bp * nchans)] = in[chan][nwritten + offset];
                else
                    d_buffer[chan + (bp * nchans)] = 0;
            }
        }
        sf_write_float(d_fp, &d_buffer[0], nchans * bp);

        errnum = sf_error(d_fp);
        if (errnum) {
            std::cerr << "sf_error: " << sf_error_number(errnum) << std::endl;
            close();
            throw std::runtime_error("File I/O error.");
        }
    }
}

void wavfile_sink_gqrx::set_sql_triggered(const bool enabled)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_squelch_triggered = enabled;
    d_prev_action = ACT_NONE;
}

void wavfile_sink_gqrx::set_bits_per_sample(int bits_per_sample)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    set_bits_per_sample_unlocked(bits_per_sample);
}

void wavfile_sink_gqrx::set_bits_per_sample_unlocked(int bits_per_sample)
{
    d_bytes_per_sample_new = bits_per_sample / 8;
}

void wavfile_sink_gqrx::set_append(bool append)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_append = append;
}

void wavfile_sink_gqrx::set_sample_rate(unsigned int sample_rate)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_h.sample_rate = sample_rate;
}

int wavfile_sink_gqrx::bits_per_sample() { return d_bytes_per_sample_new; }

unsigned int wavfile_sink_gqrx::sample_rate() { return d_h.sample_rate; }

void wavfile_sink_gqrx::do_update()
{
    if (!d_updated)
        return;

    if (d_fp)
        close_wav();

    d_fp = d_new_fp; // install new file pointer
    d_new_fp = nullptr;

    d_h.bytes_per_sample = d_bytes_per_sample_new;
    // Avoid deadlock.
    set_bits_per_sample_unlocked(8 * d_bytes_per_sample_new);
    d_updated = false;
}

void wavfile_sink_gqrx::set_rec_min_time(int min_time_ms)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_min_time_ms = min_time_ms;
    d_min_time_samp = d_min_time_ms * d_h.sample_rate / 1000;
/*    int new_history = 1 + (d_min_time_ms + d_max_gap_ms) * d_h.sample_rate / 1000;
    if (int(history()) < new_history)
        set_history(new_history);*/
}

void wavfile_sink_gqrx::set_rec_max_gap(int max_gap_ms)
{
    std::unique_lock<std::mutex> guard(d_mutex);
    d_max_gap_ms = max_gap_ms;
    d_max_gap_samp = max_gap_ms * d_h.sample_rate / 1000;
/*    int new_history = 1 + (d_min_time_ms + d_max_gap_ms) * d_h.sample_rate / 1000;
    if (int(history()) < new_history)
        set_history(new_history);*/
}

int wavfile_sink_gqrx::get_min_time()
{
    return d_min_time_ms;
}

int wavfile_sink_gqrx::get_max_gap()
{
    return d_max_gap_ms;
}
