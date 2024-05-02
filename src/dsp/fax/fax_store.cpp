/*
 * fax decoded lines sink block implementation
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

#include <gnuradio/io_signature.h>
#include "fax_store.h"

#if defined(DEBUG) && 0
#include <stdio.h>
#define D(...)    do { fprintf(stderr,__VA_ARGS__); } while (0)
#else
#define D(...)
#endif

namespace gr {
	namespace fax {

		fax_store::sptr fax_store::make(int len) {
			return gnuradio::get_initial_sptr(new gr::fax::fax_store(len));
		}

		fax_store::fax_store(int len) : gr::sync_block ("fax_store",
				gr::io_signature::make (1, 1, sizeof(float)),
				gr::io_signature::make (0, 0, 0)) {
			current_line.reserve(1810);
		}

		fax_store::~fax_store () {
		}

		void fax_store::reset() {
			std::lock_guard<std::mutex> lock(d_mutex);
			current_line.clear();
			while (!d_data.empty())
				d_data.pop();
		}

		int fax_store::work (int noutput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items) {

			std::lock_guard<std::mutex> lock(d_mutex);
			const float *in = reinterpret_cast<const float*>(input_items[0]);
			float Gray;
			int i=0;

			while (i < noutput_items) {

				if (*in == -INFINITY) { // End of line
					d_data.push(current_line);
					current_line.clear();
				} else if (*in == +INFINITY) { // End of image
					d_data.push(current_line);
					current_line.clear();
					// Insert empty line
					d_data.push(current_line);
				} else {
					Gray = roundf(((*in)*128+128));
					if (Gray < 0 ) Gray = 0;
					else if (Gray > 255) Gray=255;
					current_line.push_back((unsigned char)Gray);
				}

				in++;
				i++;
			}

			consume_each(noutput_items);

			return 0;
		}

		int fax_store::get_data (unsigned char* data, unsigned int* len) {

			std::lock_guard<std::mutex> lock(d_mutex);

			if (!d_data.empty()) {
				std::vector<unsigned char>  &line = d_data.front();

				if (*len > line.size())
					*len = line.size();

				if (data) {
					memcpy(data,line.data(),*len);
					d_data.pop();
				}

				if (d_data.empty()) {
					return -1;
				}

				// Announce next line size
				*len = d_data.front().size();

				return 0;
			}

			return -1;
		}
	}}
