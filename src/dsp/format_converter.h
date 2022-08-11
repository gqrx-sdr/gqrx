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
#include <array>

namespace dispatcher
{
    template <typename> struct tag{};
}

enum file_formats {
    FILE_FORMAT_LAST=0,
    FILE_FORMAT_NONE,
    FILE_FORMAT_CF,
    FILE_FORMAT_SIGMF,
    FILE_FORMAT_CS8,
    FILE_FORMAT_CS16L,
    FILE_FORMAT_CS32L,
    FILE_FORMAT_CS8U,
    FILE_FORMAT_CS16LU,
    FILE_FORMAT_CS32LU,
    FILE_FORMAT_CS10L,
    FILE_FORMAT_CS12L,
    FILE_FORMAT_CS14L,
    FILE_FORMAT_COUNT,
};

class any_to_any_base: virtual public gr::sync_block
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<any_to_any_base> sptr;
#else
    typedef std::shared_ptr<any_to_any_base> sptr;
#endif

    struct format_descriptor
    {
        int size;
        int nsamples;
        const char * suffix;
        const char * name;
    };

    static constexpr std::array<format_descriptor, FILE_FORMAT_COUNT> fmt
    {
        (format_descriptor){0,0,nullptr,nullptr},
        {0,0,nullptr,nullptr},
        {8,1,"fc.raw","gr_complex cf"},
        {8,1,"fc.sigmf-data","SIGMF"},
        {2,1,"8.raw","char 8"},
        {4,1,"16.raw","short 16"},
        {8,1,"32.raw","int 32"},
        {2,1,"8u.raw","uchar 8"},
        {4,1,"16u.raw","ushort 16"},
        {8,1,"32u.raw","uint 32"},
        {5,2,"10.raw","10 bit"},
        {3,1,"12.raw","12 bit"},
        {7,2,"14.raw","14 bit"},
    };

    void set_decimation(unsigned decimation)
    {
        d_decimation=decimation;
    }

    void set_interpolation(unsigned interpolation)
    {
        d_interpolation=interpolation;
    }

    unsigned decimation()
    {
        return d_decimation;
    }

    unsigned interpolation()
    {
        return d_interpolation;
    }

    int fixed_rate_ninput_to_noutput(int ninput) override
    {
        return std::max(0, ninput - (int)history() + 1) * interpolation() / decimation();
    }

    int fixed_rate_noutput_to_ninput(int noutput) override
    {
        return noutput * decimation() / interpolation() + history() - 1;
    }

    virtual void convert(const void *in, void *out, int noutput_items) = 0;

    int general_work(int noutput_items,
                                    gr_vector_int& ninput_items,
                                    gr_vector_const_void_star& input_items,
                                    gr_vector_void_star& output_items) override
    {
        int r = work(noutput_items, input_items, output_items);
        if (r > 0)
            consume_each(r * decimation() / interpolation());
        return r;
    }
    protected:
    float d_scale{1.f};
    float d_scale_i{1.f};

    private:
    unsigned d_decimation;
    unsigned d_interpolation;
};

class any_to_any_impl : virtual public gr::sync_block, virtual public any_to_any_base
{
public:
    using any_to_any_base::convert;
protected:
    void convert(const gr_complex *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<int32_t> * out, int noutput_items);
    void convert(const std::complex<int32_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<uint32_t> * out, int noutput_items);
    void convert(const std::complex<uint32_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<int16_t> * out, int noutput_items);
    void convert(const std::complex<int16_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<uint16_t> * out, int noutput_items);
    void convert(const std::complex<uint16_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<int8_t> * out, int noutput_items);
    void convert(const std::complex<int8_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::complex<uint8_t> * out, int noutput_items);
    void convert(const std::complex<uint8_t> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::array<int8_t,5> * out, int noutput_items);
    void convert(const std::array<int8_t,5> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::array<int8_t,3> * out, int noutput_items);
    void convert(const std::array<int8_t,3> *in, gr_complex * out, int noutput_items);
    void convert(const gr_complex *in, std::array<int8_t,7> * out, int noutput_items);
    void convert(const std::array<int8_t,7> *in, gr_complex * out, int noutput_items);
};

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
template <typename T_IN, typename T_OUT> class BLOCKS_API any_to_any : virtual public gr::sync_block, virtual public any_to_any_base, virtual private any_to_any_impl
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<any_to_any<T_IN, T_OUT>> sptr;
#else
    typedef std::shared_ptr<any_to_any<T_IN, T_OUT>> sptr;
#endif


    /*!
    * Build a any-to-any.
    */
    static sptr make(const double scale)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(scale));
    }

    static sptr make()
    {
        return make(dispatcher::tag<any_to_any>());
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(1.0));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,5>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(512.0f,2,1));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,5>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(512.0f,1,2));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,3>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,3>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,7>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,2,1));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,7>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,1,2));
    }

    any_to_any(const double scale, unsigned decimation=1, unsigned interpolation=1):sync_block("any_to_any",
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        d_scale = scale;
        d_scale_i = 1. / scale;
        set_decimation(decimation);
        set_interpolation(interpolation);
        set_output_multiple(interpolation);
    }
    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items) override
    {
        return work(noutput_items, input_items, output_items, dispatcher::tag<any_to_any>());
    }

    void convert(const void *in, void *out, int noutput_items) override
    {
        convert(in, out, noutput_items, dispatcher::tag<any_to_any>());
    }

private:
    template<typename U_IN, typename U_OUT> int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items, dispatcher::tag<any_to_any<U_IN, U_OUT>>)
    {
        const U_IN *in = (const U_IN *) input_items[0];
        U_OUT *out = (U_OUT *) output_items[0];

        any_to_any_impl::convert(in, out, noutput_items);
        return noutput_items;
    }

    template<typename U_IN, typename U_OUT> void convert(const void *in, void *out, int noutput_items, dispatcher::tag<any_to_any<U_IN, U_OUT>>)
    {
        const U_IN *u_in = (const U_IN *) in;
        U_OUT *u_out = (U_OUT *) out;

        any_to_any_impl::convert(u_in, u_out, noutput_items);
    }

};

#endif /* INCLUDED_FORMAT_CONVERTER_H */
