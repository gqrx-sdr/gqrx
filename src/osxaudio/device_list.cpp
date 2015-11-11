/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2014 Alexandru Csete OZ9AEC.
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
#include <string>
#include <vector>
#include <gnuradio/audio/osx_impl.h>
#include "device_list.h"


osxaudio_device::osxaudio_device(unsigned int idx, string name, string desc) :
    d_index(idx), d_name(name), d_description(desc)
{
}

osxaudio_device::~osxaudio_device()
{
}


osxaudio_device_list::osxaudio_device_list()
{
    populate_device_list();
}

osxaudio_device_list::~osxaudio_device_list()
{
    d_sources.clear();
    d_sinks.clear();
}

/** \brief Populate osx audio device list.
  *
  * This method iterates through all the input and output decives
  * and stores them for later retrieval using getInputDevices() and
  * getOutputDevices().
  */
int osxaudio_device_list::populate_device_list()
{
    std::vector<AudioDeviceID> device_ids;
    std::vector<std::string> device_names;

    int     i, num_dev, err;

    // Get input devices
    gr::audio::osx::find_audio_devices("", true, &device_ids, &device_names);
    num_dev = device_ids.size();

    if (num_dev < 0)
    {
        std::cerr << "ERROR: gr::audio::find_audio_devices() returned " << num_dev << std::endl;
        err = num_dev;
        goto error;
    }
    std::cout << "Number of audio input devices: " << num_dev << std::endl;

    for (i = 0; i < num_dev; i++)
    {
        add_source(i, device_names.at(i), device_names.at(i));
    }
    device_ids.clear();
    device_names.clear();

    // Get output devices
    gr::audio::osx::find_audio_devices("", false, &device_ids, &device_names);
    num_dev = device_ids.size();

    if (num_dev < 0)
    {
        std::cerr << "ERROR: gr::audio::find_audio_devices() returned " << num_dev << std::endl;
        err = num_dev;
        goto error;
    }
    std::cout << "Number of audio output devices: " << num_dev << std::endl;

    for (i = 0; i < num_dev; i++)
    {
        add_sink(i, device_names.at(i), device_names.at(i));
    }
    device_ids.clear();
    device_names.clear();

    return 0;

error:
    std::cerr << "An error occured while using OSX audio" << std::endl;
    std::cerr << "Error number: " << err << std::endl;
    return err;
}


void osxaudio_device_list::add_sink(unsigned int idx, string name, string desc)
{
    d_sinks.push_back(osxaudio_device(idx, name, desc));
}


void osxaudio_device_list::add_source(unsigned int idx, string name, string desc)
{
    d_sources.push_back(osxaudio_device(idx, name, desc));
}
