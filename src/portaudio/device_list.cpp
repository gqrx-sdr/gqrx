/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Alexandru Csete OZ9AEC.
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
#include <iostream>
#include <portaudio.h>
#include <string>
#include <vector>

#include "device_list.h"

using namespace std;

portaudio_device::portaudio_device(unsigned int idx, string name, string desc) :
    d_index(idx), d_name(name), d_description(desc)
{
}

portaudio_device::~portaudio_device()
{
}


portaudio_device_list::portaudio_device_list()
{
    populate_device_list();
}

portaudio_device_list::~portaudio_device_list()
{
    d_sources.clear();
    d_sinks.clear();
}

/** \brief Populate portaudioaudio device list.
  *
  * This method iterates through all the input and output decives
  * and stores them for later retrieval using getInputDevices() and
  * getOutputDevices().
  */
int portaudio_device_list::populate_device_list()
{
    int     i, num_dev;
    const   PaDeviceInfo *dev_info;
    PaError err;

    std::cout << Pa_GetVersionText() << " (version " << Pa_GetVersion() << ")" << std::endl;
            
    num_dev = Pa_GetDeviceCount();
    if (num_dev < 0)
    {
        std::cerr << "ERROR: Pa_GetDeviceCount returned " << num_dev << std::endl;
        err = num_dev;
        goto error;
    }

    std::cout << "Number of audio devices: " << num_dev << std::endl;

    for (i = 0; i < num_dev; i++)
    {
        dev_info = Pa_GetDeviceInfo(i);

        std::cout << "  " << i << ":  " << dev_info->name
                  << "  I:" << dev_info->maxInputChannels
                  << "  O:" << dev_info->maxOutputChannels
                  << std::endl;

        if (dev_info->maxInputChannels > 0)
        {
            add_source(i, dev_info->name, dev_info->name);
        }

        if (dev_info->maxOutputChannels > 0)
        {
            add_sink(i, dev_info->name, dev_info->name);
        }
    }

    return 0;

error:
    std::cerr << "An error occured while using the portaudio stream" << std::endl;
    std::cerr << "Error number: " << err << std::endl;
    std::cerr << "Error message: " << Pa_GetErrorText(err) << std::endl;
    return err;
}


void portaudio_device_list::add_sink(unsigned int idx, string name, string desc)
{
    d_sinks.push_back(portaudio_device(idx, name, desc));
}


void portaudio_device_list::add_source(unsigned int idx, string name, string desc)
{
    d_sources.push_back(portaudio_device(idx, name, desc));
}

PaDeviceIndex portaudio_device_list::get_output_device_index(const string name) const
{
    vector<portaudio_device>::const_iterator      it;

    if (name.empty())
        return -1;

    for (it = d_sinks.begin(); it < d_sinks.end(); it++)
    {
        if (it->get_name() == name)
            return it->get_index();
    }

    return -1;
}
