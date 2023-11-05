/*
 * rtty demodulator block header
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

#ifndef RTTY_DEMOD_H
#define RTTY_DEMOD_H

#include <gnuradio/hier_block2.h>
#include <vector>
#include <dsp/rtty/char_store.h>
#include <dsp/rtty/fsk_demod.h>
#include <dsp/rtty/async_rx.h>

namespace gr {
   namespace rtty {
      /*! \brief RTTY demodulator.
       *  \ingroup DSP
       *
       * This class implements the RTTY demodulator.
       *
       */
      class rtty_demod : public gr::hier_block2 {

         public:
            enum rtty_mode {
               RTTY_MODE_5BITS_BAUDOT = 0,
               RTTY_MODE_7BITS_ASCII,
               RTTY_MODE_8BITS_ASCII
            };

            enum rtty_parity {
               RTTY_PARITY_NONE = 0,
               RTTY_PARITY_ODD,
               RTTY_PARITY_EVEN,
               RTTY_PARITY_MARK,
               RTTY_PARITY_SPACE,
               RTTY_PARITY_DONTCARE
            };

#if GNURADIO_VERSION < 0x030900
            typedef boost::shared_ptr<rtty_demod> sptr;
#else
            typedef std::shared_ptr<rtty_demod> sptr;
#endif


            /*! \brief Return a shared_ptr to a new instance of rtty_demod.
             *  \param quad_rate The input sample rate.
             *  \param mark_freq The MARK frequency
             *  \param space_freq The SPACE freqency
             *  \param baud_rate The baudrate of the signal
             *
             * This is effectively the public constructor.
             */
            static sptr make(float quad_rate, float mark_freq, float space_freq,float baud_rate,enum rtty_mode mode,enum rtty_parity parity);


            void  set_quad_rate(float quad_rate);
            float quad_rate() const;
            // FSK parameters
            void  set_mark_freq(float mark_freq);
            float mark_freq();
            void  set_space_freq(float space_freq);
            float space_freq();
            void set_threshold(float threshold);
            float threshold();
            void set_cd_len(float cd_len);
            float cd_len() const;
            void set_bandwidth(float bw);
            float bandwidth() const;
            void set_transwidth(float trw);
            float transwidth() const;
            void set_filterlen(float filterlen);
            float filterlen() const;
            void  set_baud_rate(float baud_rate);
            float baud_rate();
            void  set_mode(enum rtty_mode mode);
            enum rtty_mode mode();
            void  set_parity(enum rtty_parity mode);
            enum rtty_parity parity();

            void reset();

            int get_data(std::string &data);

         private:

            rtty_demod(float quad_rate, float mark_freq, float space_freq,float baud_rate,enum rtty_mode mode, enum rtty_parity parity);
            ~rtty_demod();

            /* GR blocks */
            gr::rtty::fsk_demod::sptr d_fsk_demod; /*! fsk demodulator */
            gr::rtty::async_rx::sptr d_async; /*! Async receiver */
            char_store::sptr d_data;    /*! char data storage */

            /* other parameters */
            float   d_quad_rate;   /*! Sample rate */
            float   d_mark_freq;   /*! MARK frequency */
            float   d_space_freq;   /*! SPACE frequency */
            float   d_baud_rate;   /*! Baud rate */
            enum rtty_mode d_mode;    /* decoder mode */
            enum rtty_parity d_parity;

            /* FIR RRC filter taps */
            std::vector<float> d_rrc_taps;   /*! tone filter taps. */
      };

   }
}
#endif // RTTY_DEMOD_H
