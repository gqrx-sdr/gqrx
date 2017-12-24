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

#define dout debug && std::cout
#define lout log && std::cout

#include "decoder_impl.h"
#include "constants.h"
#include <gnuradio/io_signature.h>

using namespace gr::rds;

decoder::sptr
decoder::make(bool log, bool debug) {
  return gnuradio::get_initial_sptr(new decoder_impl(log, debug));
}

decoder_impl::decoder_impl(bool log, bool debug)
	: gr::sync_block ("gr_rds_decoder",
			gr::io_signature::make (1, 1, sizeof(char)),
			gr::io_signature::make (0, 0, 0)),
	log(log),
	debug(debug)
{
    bit_counter = 0;
    lastseen_offset_counter = 0;
    reg = 0;
    block_bit_counter = 0;
    wrong_blocks_counter = 0;
    blocks_counter = 0;
    group_good_blocks_counter = 0;
    good_block = false;
    group_assembly_started = false;
    lastseen_offset = 0;
    block_number = 0;

    set_output_multiple(104);  // 1 RDS datagroup = 104 bits
	message_port_register_out(pmt::mp("out"));
	enter_no_sync();
}

decoder_impl::~decoder_impl() {
}


////////////////////////// HELPER FUNTIONS /////////////////////////

void decoder_impl::enter_no_sync() {
	presync = false;
	d_state = NO_SYNC;
}

void decoder_impl::enter_sync(unsigned int sync_block_number) {
	wrong_blocks_counter   = 0;
	blocks_counter         = 0;
	block_bit_counter      = 0;
	block_number           = (sync_block_number + 1) % 4;
	group_assembly_started = false;
	d_state                = SYNC;
}

/* see Annex B, page 64 of the standard */
unsigned int decoder_impl::calc_syndrome(unsigned long message,
		unsigned char mlen) {
	unsigned long reg = 0;
	unsigned int i;
	const unsigned long poly = 0x5B9;
	const unsigned char plen = 10;

	for (i = mlen; i > 0; i--)  {
		reg = (reg << 1) | ((message >> (i-1)) & 0x01);
		if (reg & (1 << plen)) reg = reg ^ poly;
	}
	for (i = plen; i > 0; i--) {
		reg = reg << 1;
		if (reg & (1<<plen)) reg = reg ^ poly;
	}
	return (reg & ((1<<plen)-1));	// select the bottom plen bits of reg
}

void decoder_impl::decode_group(unsigned int *group) {
	pmt::pmt_t data = pmt::make_blob(group, 4 * sizeof(group));
	message_port_pub(pmt::mp("out"), data);
}

int decoder_impl::work (int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items)
{
	const bool *in = (const bool *) input_items[0];
    (void) output_items;

	dout << "RDS data decoder at work: input_items = "
		<< noutput_items << ", /104 = "
		<< noutput_items / 104 << std::endl;

	int i=0,j;
	unsigned long bit_distance, block_distance;
	unsigned int block_calculated_crc, block_received_crc, checkword,dataword;
	unsigned int reg_syndrome;

/* the synchronization process is described in Annex C, page 66 of the standard */
	while (i<noutput_items) {
		reg=(reg<<1)|in[i];		// reg contains the last 26 rds bits
		switch (d_state) {
			case NO_SYNC:
				reg_syndrome = calc_syndrome(reg,26);
				for (j=0;j<5;j++) {
					if (reg_syndrome==syndrome[j]) {
						if (!presync) {
							lastseen_offset=j;
							lastseen_offset_counter=bit_counter;
							presync=true;
						}
						else {
							bit_distance=bit_counter-lastseen_offset_counter;
							if (offset_pos[lastseen_offset]>=offset_pos[j]) 
								block_distance=offset_pos[j]+4-offset_pos[lastseen_offset];
							else
								block_distance=offset_pos[j]-offset_pos[lastseen_offset];
							if ((block_distance*26)!=bit_distance) presync=false;
							else {
								lout << "@@@@@ Sync State Detected" << std::endl;
								enter_sync(j);
							}
						}
					break; //syndrome found, no more cycles
					}
				}
			break;
			case SYNC:
/* wait until 26 bits enter the buffer */
				if (block_bit_counter<25) block_bit_counter++;
				else {
					good_block=false;
					dataword=(reg>>10) & 0xffff;
					block_calculated_crc=calc_syndrome(dataword,16);
					checkword=reg & 0x3ff;
/* manage special case of C or C' offset word */
					if (block_number==2) {
						block_received_crc=checkword^offset_word[block_number];
						if (block_received_crc==block_calculated_crc)
							good_block=true;
						else {
							block_received_crc=checkword^offset_word[4];
							if (block_received_crc==block_calculated_crc)
								good_block=true;
							else {
								wrong_blocks_counter++;
								good_block=false;
							}
						}
					}
					else {
						block_received_crc=checkword^offset_word[block_number];
						if (block_received_crc==block_calculated_crc)
							good_block=true;
						else {
							wrong_blocks_counter++;
							good_block=false;
						}
					}
/* done checking CRC */
					if (block_number==0 && good_block) {
						group_assembly_started=true;
						group_good_blocks_counter=1;
					}
					if (group_assembly_started) {
						if (!good_block) group_assembly_started=false;
						else {
							group[block_number]=dataword;
							group_good_blocks_counter++;
						}
						if (group_good_blocks_counter==5) decode_group(group);
					}
					block_bit_counter=0;
					block_number=(block_number+1) % 4;
					blocks_counter++;
/* 1187.5 bps / 104 bits = 11.4 groups/sec, or 45.7 blocks/sec */
					if (blocks_counter==50) {
						if (wrong_blocks_counter>35) {
							lout << "@@@@@ Lost Sync (Got " << wrong_blocks_counter
								<< " bad blocks on " << blocks_counter
								<< " total)" << std::endl;
							enter_no_sync();
						} else {
							lout << "@@@@@ Still Sync-ed (Got " << wrong_blocks_counter
								<< " bad blocks on " << blocks_counter
								<< " total)" << std::endl;
						}
						blocks_counter=0;
						wrong_blocks_counter=0;
					}
				}
			break;
			default:
				d_state=NO_SYNC;
			break;
		}
		i++;
		bit_counter++;
	}
	return noutput_items;
}
