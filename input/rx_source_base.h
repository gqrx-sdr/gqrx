/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#ifndef RX_SOURCE_BASE_H
#define RX_SOURCE_BASE_H

#include <gr_hier_block2.h>


class rx_source_base;

typedef boost::shared_ptr<rx_source_base> rx_source_base_sptr;


/*! \brief Base class for all types of input source.
 *  \ingroup DSP
 *
 * This block provides a base class for signal sources. All sources
 * should be derived from this class to ensure uniform API.
 *
 */
class rx_source_base : public gr_hier_block2
{

public:
    /*! \brief Public contructor.
     *  \param src_name Descriptive name used in the contructor of gr_hier_block2
     */
    rx_source_base(const std::string src_name);
    ~rx_source_base();

    /*! \brief Select a new device.
     *  \param device The new device (format depends on source type).
     *
     * This method can be used to select a new input device. For the FCD source
     * this is the audio input device, for UHD it is the sub-device specification.
     */
    virtual void select_device(const std::string device) = 0;

    /*! \brief Set center frequency.
     *  \param freq The new center frequency in Hz.
     */
    virtual void set_freq(double freq) = 0;

    /*! \brief Get center frequency.
     *  \returns The current center frequency.
     */
    virtual double get_freq() = 0;
    virtual double get_freq_min() = 0;
    virtual double get_freq_max() = 0;

    virtual void set_gain(double gain) = 0;
    virtual double get_gain() = 0;
    virtual double get_gain_min() = 0;
    virtual double get_gain_max() = 0;

    virtual void set_sample_rate(double sps) = 0;
    virtual double get_sample_rate() = 0;
    virtual std::vector<double> get_sample_rates() = 0;

    virtual void set_freq_corr(int ppm) = 0;
    virtual void set_dc_corr(double dci, double dcq) = 0;
    virtual void set_iq_corr(double gain, double phase) = 0;
};

#endif // RX_SOURCE_BASE_H
