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
#ifndef INCLUDED_FORMAT_CONVERTER_H
#define INCLUDED_FORMAT_CONVERTER_H

#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_interpolator.h>
#include <volk/volk.h>
#include <iostream>

namespace dispatcher
{
    template <typename> struct tag{};
}

    /*!
     * \brief Convert stream of one format to a stream of another format.
     * \ingroup type_converters_blk
     *
     * \details
     * The output stream contains chars with twice as many output
     * items as input items. For every complex input item, we produce
     * two output chars that contain the real part and imaginary part
     * converted to chars:
     *
     * \li output[0][n] = static_cast<char>(input[0][m].real());
     * \li output[0][n+1] = static_cast<char>(input[0][m].imag());
     */
template <typename T_IN,typename T_OUT> class BLOCKS_API any_to_any : virtual public gr::sync_interpolator
{
public:
    typedef boost::shared_ptr<any_to_any<T_IN,T_OUT>> sptr;

    /*!
    * Build a any-to-any.
    */
    static sptr make(const double scale)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN,T_OUT>(scale));
    }
    any_to_any(const double scale):sync_interpolator("any_to_any",
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)),1)
    {
        //d_vector=false;
            std::cerr<<__FUNCTION__<<std::endl;
        d_scale=scale;
    }
    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items)
    {
        return work(noutput_items,input_items,output_items,dispatcher::tag<any_to_any>());
    }

private:
    float d_scale;

    template<typename U_IN,typename U_OUT> int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<U_IN,U_OUT>>)
    {
        return noutput_items;
    }

    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<gr_complex,gr_complex>>)
    {
        const T_IN *in = (const T_IN *) input_items[0];
        T_OUT *out = (T_OUT *) output_items[0];

        memcpy(out,in,noutput_items*2*sizeof(gr_complex));
        return noutput_items;
    }


    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<gr_complex,std::complex<short>>>)
    {
        const float *in = (const float *) input_items[0];
        short *out = (short *) output_items[0];

        volk_32f_s32f_convert_16i(out,in,d_scale,noutput_items*2);
        return noutput_items;
    }

    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<gr_complex,std::complex<char>>>)
    {
        const float *in = (const float *) input_items[0];
        signed char *out = (signed char *) output_items[0];

        volk_32f_s32f_convert_8i(out,in,d_scale,noutput_items*2);
        return noutput_items;
    }

    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<std::complex<short>,gr_complex>>)
    {
        const short *in = (const short *) input_items[0];
        float *out = (float *) output_items[0];

        volk_16i_s32f_convert_32f(out,in,d_scale,noutput_items*2);
        return noutput_items;
    }

    int work(int noutput_items,gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items,dispatcher::tag<any_to_any<std::complex<char>,gr_complex>>)
    {
        const signed char *in = (const signed char *) input_items[0];
        float *out = (float *) output_items[0];

        volk_8i_s32f_convert_32f(out,in,d_scale,noutput_items*2);
        return noutput_items;
    }
};




#endif /* INCLUDED_FORMAT_CONVERTER_H */
