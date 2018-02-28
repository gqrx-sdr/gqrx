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
#pragma once
#include <string>
#include <vector>
#include <portaudio.h>

using namespace std;

/*! \brief Simple class to represent portaudio devices.
 *
 * This class represents a portaudio device. The device can be either source
 * or sink, this class does not differentiate between them.
 */
class portaudio_device
{
public:
    portaudio_device(unsigned int idx=0, string name="", string desc="");
    ~portaudio_device();

    void set_index(unsigned int idx) { d_index = idx; }
    void set_name(string name) { d_name = name; }
    void set_description(string desc) { d_description = desc; }

    unsigned int get_index() const { return d_index; }
    string  get_name() const { return d_name; }
    string  get_description() const { return d_description; }

private:
    unsigned int d_index;    /*! The index of the audio device (unique for each source/sink). */
    string  d_name;           /*! The name of the audio device. Used when creating souces/sinks. */
    string  d_description;    /*! The description of the audio device. */
};


/*! \brief Class for storing and retrieving a list of portaudio sinks and sources. */
class portaudio_device_list
{
public:
    portaudio_device_list();
    ~portaudio_device_list();

    vector<portaudio_device> get_input_devices() { return d_sources; }
    vector<portaudio_device> get_output_devices() {return d_sinks; }

    /** Get output device index. Returns -1 if not found */
    PaDeviceIndex   get_output_device_index(const string name) const;

private:
    vector<portaudio_device> d_sources;   /*! List of pulseaudio sources. */
    vector<portaudio_device> d_sinks;  /*! List of pulseaudio sinks. */

    int populate_device_list();
    void add_sink(unsigned int idx, string name, string desc);
    void add_source(unsigned int idx, string name, string desc);
};
