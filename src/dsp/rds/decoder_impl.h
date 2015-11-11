/*
 * Copyright (C) 2014 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDED_RDS_DECODER_IMPL_H
#define INCLUDED_RDS_DECODER_IMPL_H

#include "dsp/rds/decoder.h"

namespace gr {
namespace rds {

class decoder_impl : public decoder
{
public:
	decoder_impl(bool log, bool debug);

private:
	~decoder_impl();

	int work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);

	void enter_no_sync();
	void enter_sync(unsigned int);
	unsigned int calc_syndrome(unsigned long, unsigned char);
	void decode_group(unsigned int*);

	unsigned long  bit_counter;
	unsigned long  lastseen_offset_counter, reg;
	unsigned int   block_bit_counter;
	unsigned int   wrong_blocks_counter;
	unsigned int   blocks_counter;
	unsigned int   group_good_blocks_counter;
	unsigned int   group[4];
	bool           log;
	bool           debug;
	bool           presync;
	bool           good_block;
	bool           group_assembly_started;
	unsigned char  lastseen_offset;
	unsigned char  block_number;
	enum { NO_SYNC, SYNC } d_state;

};

} /* namespace rds */
} /* namespace gr */

#endif /* INCLUDED_RDS_DECODER_IMPL_H */

