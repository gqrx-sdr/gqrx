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

#include "parser_impl.h"
#include "constants.h"
#include "tmc_events.h"
#include <gnuradio/io_signature.h>
#include <math.h>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <limits>

using namespace gr::rds;

parser::sptr
parser::make(bool log, bool debug, unsigned char pty_locale) {
	return gnuradio::get_initial_sptr(new parser_impl(log, debug, pty_locale));
}

parser_impl::parser_impl(bool log, bool debug, unsigned char pty_locale)
	: gr::block ("gr_rds_parser",
			gr::io_signature::make (0, 0, 0),
			gr::io_signature::make (0, 0, 0)),
	log(log),
	debug(debug),
	pty_locale(pty_locale),
	free_format{0, 0, 0, 0},
	no_groups(0),
	ps_on{' ',' ',' ',' ',' ',' ',' ',' '}
{
	message_port_register_in(pmt::mp("in"));
	set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { this->parse(msg); });
	message_port_register_out(pmt::mp("out"));
	reset();
}

parser_impl::~parser_impl() {
}

void parser_impl::reset() {
	gr::thread::scoped_lock lock(d_mutex);

	radiotext_segment_flags            = 0;
	program_service_name_segment_flags = 0;
	af_pairs.clear();

	radiotext_AB_flag              = 0;
	traffic_program                = false;
	traffic_announcement           = false;
	music_speech                   = false;
	program_identification         = UINT_MAX;
	program_type                   = 0;
	pi_country_identification      = 0;
	pi_area_coverage               = 0;
	pi_program_reference_number    = 0;
	mono_stereo                    = false;
	artificial_head                = false;
	compressed                     = false;
	dynamic_pty                    = false;
}

/* type 0 = PI
 * type 1 = PS
 * type 2 = PTY
 * type 3 = flagstring: TP, TA, MuSp, MoSt, AH, CMP, stPTY
 * type 4 = RadioText
 * type 5 = ClockTime
 * type 6 = Alternative Frequencies */
void parser_impl::send_message(long msgtype, std::string msgtext) {
	pmt::pmt_t msg  = pmt::mp(msgtext);
	pmt::pmt_t type = pmt::from_long(msgtype);
	message_port_pub(pmt::mp("out"), pmt::make_tuple(type, msg));
}

/* BASIC TUNING: see page 21 of the standard */
void parser_impl::decode_type0(unsigned int *group, bool B) {
	char flagstring[8]     = "0000000";

	traffic_program        = (group[1] >> 10) & 0x01;       // "TP"
	traffic_announcement   = (group[1] >>  4) & 0x01;       // "TA"
	music_speech           = (group[1] >>  3) & 0x01;       // "MuSp"

	bool decoder_control_bit      = (group[1] >> 2) & 0x01; // "DI"
	unsigned char segment_address =  group[1] & 0x03;       // "DI segment"

	char ps_1 = (group[3] >> 8) & 0xff;
	char ps_2 =  group[3]       & 0xff;

	if (program_service_name_segment_flags & (1 << segment_address)) {
		// Already received this segment. Check whether the characters have changed.
		if ((program_service_name[segment_address * 2] != ps_1) 
				|| (program_service_name[segment_address * 2 + 1] != ps_2)) {
			// The characters changed, so reset and start from scratch.
			program_service_name[segment_address * 2]     = ps_1;
			program_service_name[segment_address * 2 + 1] = ps_2;
			program_service_name_segment_flags = (1 << segment_address);
		}
	} else {
		// New segment received. Store it.
		program_service_name[segment_address * 2]     = ps_1;
		program_service_name[segment_address * 2 + 1] = ps_2;
		program_service_name_segment_flags |= (1 << segment_address);

		// If we now have all segments, report the full program service name.
		if (program_service_name_segment_flags == 0xf) {
			send_message(1, std::string(program_service_name, 8));
		}
	}

	/* see page 41, table 9 of the standard */
	switch (segment_address) {
		case 0:
			dynamic_pty=decoder_control_bit;
		break;
		case 1:
			compressed=decoder_control_bit;
		break;
		case 2:
			artificial_head=decoder_control_bit;
		break;
		case 3:
			mono_stereo=decoder_control_bit;
		break;
		default:
		break;
	}
	flagstring[0] = traffic_program        ? '1' : '0';
	flagstring[1] = traffic_announcement   ? '1' : '0';
	flagstring[2] = music_speech           ? '1' : '0';
	flagstring[3] = mono_stereo            ? '1' : '0';
	flagstring[4] = artificial_head        ? '1' : '0';
	flagstring[5] = compressed             ? '1' : '0';
	flagstring[6] = dynamic_pty            ? '1' : '0';

	if(!B) { // type 0A
		// Check whether this is a new pair of AF codes
		if (std::find(af_pairs.begin(), af_pairs.end(), group[2]) == af_pairs.end()) {
			af_pairs.push_back(group[2]);
			decode_af_pairs();
		}
	}

	lout << "==>" << std::string(program_service_name, 8)
		<< "<== -" << (traffic_program ? "TP" : "  ")
		<< '-' << (traffic_announcement ? "TA" : "  ")
		<< '-' << (music_speech ? "Music" : "Speech")
		<< '-' << (mono_stereo ? "STEREO" : "MONO")
		<< std::endl;

	send_message(3, flagstring);
}

void parser_impl::decode_af_pairs() {
	// Search for the first row, which indicates the number of frequencies
	int first_row = -1;
	int number_of_freqs = -1;
	int freqs_seen = 0;
	std::vector<int> freqs;

	for (unsigned int i = 0; i < af_pairs.size(); i++) {
		unsigned int af_1 = (af_pairs[i] >> 8);

		if ((af_1 >= 224) && (af_1 <= 249)) {
			first_row = i;
			number_of_freqs = af_1 - 224;
			break;
		}
	}

	if (number_of_freqs == 0) {
		return;
	}

	if (first_row >= 0) {
		unsigned int special_af = 0;
		for (unsigned int i = first_row; i < first_row + af_pairs.size(); i++) {
			unsigned int af_1 = (af_pairs[i % af_pairs.size()] >> 8);
			unsigned int af_2 = (af_pairs[i % af_pairs.size()] & 0xff);

			if ((int)i == first_row) {
				int freq = decode_af(af_2, false);
				if (freq >= 0) {
					freqs.push_back(freq);
					freqs_seen++;
					special_af = af_2;
				}
			} else if (af_1 == 250) {
				// One LF/MF frequency follows
				int freq = decode_af(af_2, true);
				if (freq >= 0) {
					freqs.push_back(freq);
					freqs_seen++;
				}
			} else {
				if ((af_1 == special_af) || (af_2 == special_af)) {
					// AF method B
					bool regional = (af_2 < af_1);

					unsigned int new_af = (af_1 == special_af) ? af_2 : af_1;
					int freq = decode_af(new_af, false);
					if (freq >= 0) {
						freqs.push_back(regional ? -freq : freq); // Negative indicates regional frequency
						freqs_seen += 2;
					}
				} else {
					int freq = decode_af(af_1, false);
					if (freq >= 0) {
						freqs.push_back(freq);
						freqs_seen++;
					}
					freq = decode_af(af_2, false);
					if (freq >= 0) {
						freqs.push_back(freq);
						freqs_seen++;
					}
				}
			}
		}

		// Check whether we have the whole frequency list
		if (freqs_seen == number_of_freqs) {
			std::stringstream af_stringstream;
			af_stringstream << std::fixed << std::setprecision(1);

			bool first = true;
			for (int freq : freqs) {
				bool regional = false;
				if (freq < 0) {
					freq = -freq;
					regional = true;
				}

				if (first) {
					first = false;
				} else {
					af_stringstream << ", ";
				}

				if (freq > 10000) {
					af_stringstream << (freq / 1000.0) << " MHz";
				} else {
					af_stringstream << freq << " kHz";
				}

				if (regional) {
					af_stringstream << " (regional)";
				}
			}

			send_message(6, af_stringstream.str());
			lout << "AF: " << af_stringstream.str() << std::endl;
		}
	}

}

int parser_impl::decode_af(unsigned int af_code, bool lf_mf) {
	if (lf_mf) {
		if ((af_code >= 1) && (af_code <= 15)) {
			return 153 + (af_code - 1) * 9; // LF (153-279kHz)
		} else if ((af_code >= 16) && (af_code < 135)) {
			return 531 + (af_code - 16) * 9; // MF (531-1602kHz)
		}
	} else {
		if ((af_code >= 1) && (af_code <= 204)) {
			return 87600 + (af_code - 1) * 100; // VHF (87.6-107.9MHz)
		}
	}

	return -1; // Invalid input
}

void parser_impl::decode_type1(unsigned int *group, bool B){
	int ecc    = 0;
	int paging = 0;
	char country_code           = (group[0] >> 12) & 0x0f;
	char radio_paging_codes     =  group[1]        & 0x1f;
	int variant_code            = (group[2] >> 12) & 0x7;
	unsigned int slow_labelling =  group[2]        & 0xfff;
	int day    = (int)((group[3] >> 11) & 0x1f);
	int hour   = (int)((group[3] >>  6) & 0x1f);
	int minute = (int) (group[3]        & 0x3f);

	if(radio_paging_codes) {
		lout << "paging codes: " << int(radio_paging_codes) << " ";
	}
	if(day || hour || minute) {
		lout << "program item: " << day << ", " << hour << ", " << minute << " ";
	}

	if(!B){
		switch(variant_code){
			case 0: // paging + ecc
				paging = (slow_labelling >> 8) & 0x0f;
				ecc    =  slow_labelling       & 0xff;
				if(paging) {
					lout << "paging: " << paging << " ";
				}
				if((ecc > 223) && (ecc < 229)) {
					lout << "extended country code: "
						<< pi_country_codes[country_code-1][ecc-224]
						<< std::endl;
				} else {
					lout << "invalid extended country code: " << ecc << std::endl;
				}
				break;
			case 1: // TMC identification
				lout << "TMC identification code received" << std::endl;
				break;
			case 2: // Paging identification
				lout << "Paging identification code received" << std::endl;
				break;
			case 3: // language codes
				if(slow_labelling < 44) {
					lout << "language: " << language_codes[slow_labelling]
						<< std::endl;
				} else {
					lout << "language: invalid language code " << slow_labelling
						<< std::endl;
				}
				break;
			default:
				break;
		}
	}
}

void parser_impl::decode_type2(unsigned int *group, bool B){
	unsigned char text_segment_address_code = group[1] & 0x0f;

	// when the A/B flag is toggled, flush your current radiotext
	if(radiotext_AB_flag != ((group[1] >> 4) & 0x01)) {
		radiotext_segment_flags = 0;
	}
	radiotext_AB_flag = (group[1] >> 4) & 0x01;

	if(!B) {
		radiotext[text_segment_address_code * 4    ] = (group[2] >> 8) & 0xff;
		radiotext[text_segment_address_code * 4 + 1] =  group[2]       & 0xff;
		radiotext[text_segment_address_code * 4 + 2] = (group[3] >> 8) & 0xff;
		radiotext[text_segment_address_code * 4 + 3] =  group[3]       & 0xff;
	} else {
		radiotext[text_segment_address_code * 2    ] = (group[3] >> 8) & 0xff;
		radiotext[text_segment_address_code * 2 + 1] =  group[3]       & 0xff;
	}
	radiotext_segment_flags |= (1 << text_segment_address_code);

	// Count how many valid segments we have.
	int valid_segments = 0;
	for (int segment_address = 0; segment_address < 16; segment_address++) {
		if (radiotext_segment_flags & (1 << segment_address)) {
			valid_segments++;
		} else {
			break;
		}
	}

	// Check for the end of the radiotext, indicated by a carriage return.
	int segment_width = (B ? 2 : 4);
	char *tail = (char *) std::memchr(radiotext, '\r', valid_segments * segment_width);

	// Special case: radiotext that take up the entire buffer is not
	// required to have a trailing carriage return.
	if (!tail && (valid_segments == 16)) {
		tail = radiotext + (16 * segment_width);
	}

	// If the radiotext is complete, report it.
	if (tail) {
		lout << "Radio Text " << (radiotext_AB_flag ? 'B' : 'A')
			<< ": " << std::string(radiotext, tail - radiotext)
			<< std::endl;
		send_message(4, std::string(radiotext, tail - radiotext));
	}
}

void parser_impl::decode_type3(unsigned int *group, bool B){
	if(B) {
		dout << "type 3B not implemented yet" << std::endl;
		return;
	}

	int application_group = (group[1] >> 1) & 0xf;
	int group_type        =  group[1] & 0x1;
	int message           =  group[2];
	int aid               =  group[3];

	lout << "aid group: " << application_group
		<< " " << (group_type ? 'B' : 'A');
	if((application_group == 8) && (group_type == false)) { // 8A
		int variant_code = (message >> 14) & 0x3;
		if(variant_code == 0) {
			int ltn  = (message >> 6) & 0x3f; // location table number
			bool afi = (message >> 5) & 0x1;  // alternative freq. indicator
			bool M   = (message >> 4) & 0x1;  // mode of transmission
			bool I   = (message >> 3) & 0x1;  // international
			bool N   = (message >> 2) & 0x1;  // national
			bool R   = (message >> 1) & 0x1;  // regional
			bool U   =  message       & 0x1;  // urban
			lout << "location table: " << ltn << " - "
				<< (afi ? "AFI-ON" : "AFI-OFF") << " - "
				<< (M   ? "enhanced mode" : "basic mode") << " - "
				<< (I   ? "international " : "")
				<< (N   ? "national " : "")
				<< (R   ? "regional " : "")
				<< (U   ? "urban" : "")
				<< " aid: " << aid << std::endl;

		} else if(variant_code==1) {
			int G   = (message >> 12) & 0x3;  // gap
			int sid = (message >>  6) & 0x3f; // service identifier
			int gap_no[4] = {3, 5, 8, 11};
			lout << "gap: " << gap_no[G] << " groups, SID: "
				<< sid << " ";
		}
	}
	lout << "message: " << message << " - aid: " << aid << std::endl;;
}

void parser_impl::decode_type4(unsigned int *group, bool B){
	if(B) {
		dout << "type 4B not implemented yet" << std::endl;
		return;
	}

	unsigned int hours   = ((group[2] & 0x1) << 4) | ((group[3] >> 12) & 0x0f);
	unsigned int minutes =  (group[3] >> 6) & 0x3f;
	double local_time_offset = .5 * (group[3] & 0x1f);

	if((group[3] >> 5) & 0x1) {
		local_time_offset *= -1;
	}
	double modified_julian_date = ((group[1] & 0x03) << 15) | ((group[2] >> 1) & 0x7fff);

	unsigned int year  = int((modified_julian_date - 15078.2) / 365.25);
	unsigned int month = int((modified_julian_date - 14956.1 - int(year * 365.25)) / 30.6001);
	unsigned int day   =               modified_julian_date - 14956 - int(year * 365.25) - int(month * 30.6001);
	bool K = ((month == 14) || (month == 15)) ? 1 : 0;
	year += K;
	month -= 1 + K * 12;

	std::stringstream time;
	time << std::setfill('0');
	time << std::setw(2) << day << ".";
	time << std::setw(2) << month << ".";
	time << std::setw(4) << (1900 + year) << ", ";
	time << std::setw(2) << hours << ":";
	time << std::setw(2) << minutes << " (";
	time << std::fixed << std::showpos << std::setprecision(1) << local_time_offset << "h)";

	lout << "Clocktime: " << time.str() << std::endl;

	send_message(5,time.str());
}

void parser_impl::decode_type5(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 5 not implemented yet" << std::endl;
}

void parser_impl::decode_type6(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 6 not implemented yet" << std::endl;
}

void parser_impl::decode_type7(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 7 not implemented yet" << std::endl;
}

void parser_impl::decode_type8(unsigned int *group, bool B){
	if(B) {
		dout << "type 8B not implemented yet" << std::endl;
		return;
	}
	bool T = (group[1] >> 4) & 0x1; // 0 = user message, 1 = tuning info
	bool F = (group[1] >> 3) & 0x1; // 0 = multi-group, 1 = single-group
	bool D = (group[2] >> 15) & 0x1; // 1 = diversion recommended

	if(T) { // tuning info
		lout << "#tuning info# ";
		int variant = group[1] & 0xf;
		if((variant > 3) && (variant < 10)) {
			lout << "variant: " << variant << " - "
				<< group[2] << " " << group[3] << std::endl;
		} else {
			lout << "invalid variant: " << variant << std::endl;
		}

	} else if(F || D) { // single-group or 1st of multi-group
		unsigned int dp_ci    =  group[1]        & 0x7;   // duration & persistence or continuity index
		bool sign             = (group[2] >> 14) & 0x1;   // event direction, 0 = +, 1 = -
		unsigned int extent   = (group[2] >> 11) & 0x7;   // number of segments affected
		unsigned int event    =  group[2]        & 0x7ff; // event code, defined in ISO 14819-2
		unsigned int location =  group[3];                // location code, defined in ISO 14819-3
		lout << "#user msg# ";
		if(F) {
			if (D) {
				lout << "diversion recommended, ";
			}
			lout << "single-grp, duration:" << tmc_duration[dp_ci][0];
		} else {
			lout << "multi-grp, continuity index:" << dp_ci;
		}
		int event_line = tmc_event_code_index[event][1];
		lout << ", extent:" << (sign ? "-" : "") << extent + 1 << " segments"
			<< ", event" << event << ":" << tmc_events[event_line][1]
			<< ", location:" << location << std::endl;

	} else { // 2nd or more of multi-group
		unsigned int ci = group[1] & 0x7;          // countinuity index
		bool sg = (group[2] >> 14) & 0x1;          // second group
		unsigned int gsi = (group[2] >> 12) & 0x3; // group sequence
		lout << "#user msg# multi-grp, continuity index:" << ci
			<< (sg ? ", second group" : "") << ", gsi:" << gsi;
		lout << ", free format: " << (group[2] & 0xfff) << " "
			<< group[3] << std::endl;
		// it's not clear if gsi=N-2 when gs=true
		if(sg) {
			no_groups = gsi;
		}
		free_format[gsi] = ((group[2] & 0xfff) << 12) | group[3];
		if(gsi == 0) {
			decode_optional_content(no_groups, free_format);
		}
	}
}

void parser_impl::decode_optional_content(int no_groups, unsigned long int *free_format){
	int label          = 0;
	int content        = 0;
	int content_length = 0;
	int ff_pointer     = 0;

	for (int i = no_groups; i == 0; i--){
		ff_pointer = 12 + 16;
		while(ff_pointer > 0){
			ff_pointer -= 4;
			label = (free_format[i] >> ff_pointer) & 0xf;
			content_length = optional_content_lengths[label];
			ff_pointer -= content_length;
			content = (free_format[i] >> ff_pointer) & ((1 << content_length) - 1);
			lout << "TMC optional content (" << label_descriptions[label]
				<< "):" << content << std::endl;
		}
	}
}

void parser_impl::decode_type9(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 9 not implemented yet" << std::endl;
}

void parser_impl::decode_type10(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 10 not implemented yet" << std::endl;
}

void parser_impl::decode_type11(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 11 not implemented yet" << std::endl;
}

void parser_impl::decode_type12(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 12 not implemented yet" << std::endl;
}

void parser_impl::decode_type13(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 13 not implemented yet" << std::endl;
}

void parser_impl::decode_type14(unsigned int *group, bool B){

	bool tp_on               = (group[1] >> 4) & 0x01;
	char variant_code        = group[1] & 0x0f;
	unsigned int information = group[2];
	unsigned int pi_on       = group[3];

	char pty_on = 0;
	bool ta_on = 0;
	double af_1 = 0;
	double af_2 = 0;

	if (!B){
		switch (variant_code){
			case 0: // PS(ON)
			case 1: // PS(ON)
			case 2: // PS(ON)
			case 3: // PS(ON)
				ps_on[variant_code * 2    ] = (information >> 8) & 0xff;
				ps_on[variant_code * 2 + 1] =  information       & 0xff;
				lout << "PS(ON): ==>" << std::string(ps_on, 8) << "<==";
			break;
			case 4: // AF
				af_1 = 100.0 * (((information >> 8) & 0xff) + 875);
				af_2 = 100.0 * ((information & 0xff) + 875);
				lout << "AF:" << std::fixed << std::setprecision(2) << (af_1/1000) << "MHz " << (af_2/1000) << "MHz";
			break;
			case 5: // mapped frequencies
			case 6: // mapped frequencies
			case 7: // mapped frequencies
			case 8: // mapped frequencies
				af_1 = 100.0 * (((information >> 8) & 0xff) + 875);
				af_2 = 100.0 * ((information & 0xff) + 875);
				lout << "TN:" << std::fixed << std::setprecision(2) << (af_1/1000) << "MHz - ON:" << (af_2/1000) << "MHz";
			break;
			case 9: // mapped frequencies (AM)
				af_1 = 100.0 * (((information >> 8) & 0xff) + 875);
				af_2 = 9.0 * ((information & 0xff) - 16) + 531;
				lout << "TN:" << std::fixed << std::setprecision(2) << (af_1/1000) << "MHz - ON:" << int(af_2) << "kHz";
			break;
			case 10: // unallocated
			break;
			case 11: // unallocated
			break;
			case 12: // linkage information
				lout << "Linkage information: " << std::hex << std::setw(4) << information << std::dec;
			break;
			case 13: // PTY(ON), TA(ON)
				ta_on = information & 0x01;
				pty_on = (information >> 11) & 0x1f;
				lout << "PTY(ON):" << pty_table[int(pty_on)][pty_locale];
				if(ta_on) {
					lout << " - TA";
				}
			break;
			case 14: // PIN(ON)
				lout << "PIN(ON):" << std::hex << std::setw(4) << information << std::dec;
			break;
			case 15: // Reserved for broadcasters use
			break;
			default:
				dout << "invalid variant code:" << variant_code;
			break;
		}
	}
	if (pi_on){
		lout << " PI(ON):" << pi_on;
		if (tp_on) {
			lout << "-TP-";
		}
	}
	lout << std::endl;
}

void parser_impl::decode_type15(unsigned int *group, bool B){
	(void) group;
	(void) B;
	dout << "type 15 not implemented yet" << std::endl;
}

void parser_impl::parse(pmt::pmt_t pdu) {
	if(!pmt::is_pair(pdu)) {
		dout << "wrong input message (not a PDU)" << std::endl;
		return;
	}

	//pmt::pmt_t meta = pmt::car(pdu);  // meta is currently not in use
	pmt::pmt_t vec = pmt::cdr(pdu);

	if(!pmt::is_blob(vec)) {
		dout << "input PDU message has wrong type (not u8)" << std::endl;
		return;
	}
	if(pmt::blob_length(vec) != 12) {  // 8 data + 4 offset chars (ABCD)
		dout << "input PDU message has wrong size ("
			<< pmt::blob_length(vec) << ")" << std::endl;
		return;
	}

	unsigned char *bytes = (unsigned char *)pmt::blob_data(vec);
	unsigned int group[4];
	group[0] = bytes[1] | (((unsigned int)(bytes[0])) << 8U);
	group[1] = bytes[3] | (((unsigned int)(bytes[2])) << 8U);
	group[2] = bytes[5] | (((unsigned int)(bytes[4])) << 8U);
	group[3] = bytes[7] | (((unsigned int)(bytes[6])) << 8U);

	// TODO: verify offset chars are one of: "ABCD", "ABcD"

	unsigned int group_type = (unsigned int)((group[1] >> 12) & 0xf);
	bool ab = (group[1] >> 11 ) & 0x1;

	lout << std::setfill('0') << std::setw(2) << group_type << (ab ? 'B' : 'A') << " ";
	lout << "(" << rds_group_acronyms[group_type] << ")";

	if (program_identification != group[0]) {
		reset();
	}
	program_identification = group[0];     // "PI"
	program_type = (group[1] >> 5) & 0x1f; // "PTY"
	int pi_country_identification = (program_identification >> 12) & 0xf;
	int pi_area_coverage = (program_identification >> 8) & 0xf;
	unsigned char pi_program_reference_number = program_identification & 0xff;
	std::stringstream pistring;
	pistring << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << program_identification;
	send_message(0, pistring.str());
	send_message(2, pty_table[program_type][pty_locale]);

	lout << " - PI:" << pistring.str() << " - " << "PTY:" << pty_table[program_type][pty_locale];
	lout << " (country:" << pi_country_codes[pi_country_identification - 1][0];
	lout << "/" << pi_country_codes[pi_country_identification - 1][1];
	lout << "/" << pi_country_codes[pi_country_identification - 1][2];
	lout << "/" << pi_country_codes[pi_country_identification - 1][3];
	lout << "/" << pi_country_codes[pi_country_identification - 1][4];
	lout << ", area:" << coverage_area_codes[pi_area_coverage];
	lout << ", program:" << int(pi_program_reference_number) << ")" << std::endl;

	switch (group_type) {
		case 0:
			decode_type0(group, ab);
			break;
		case 1:
			decode_type1(group, ab);
			break;
		case 2:
			decode_type2(group, ab);
			break;
		case 3:
			decode_type3(group, ab);
			break;
		case 4:
			decode_type4(group, ab);
			break;
		case 5:
			decode_type5(group, ab);
			break;
		case 6:
			decode_type6(group, ab);
			break;
		case 7:
			decode_type7(group, ab);
			break;
		case 8:
			decode_type8(group, ab);
			break;
		case 9:
			decode_type9(group, ab);
			break;
		case 10:
			decode_type10(group, ab);
			break;
		case 11:
			decode_type11(group, ab);
			break;
		case 12:
			decode_type12(group, ab);
			break;
		case 13:
			decode_type13(group, ab);
			break;
		case 14:
			decode_type14(group, ab);
			break;
		case 15:
			decode_type15(group, ab);
			break;
	}

	#define HEX(a) std::hex << std::setfill('0') << std::setw(4) << long(a) << std::dec
	for(int i = 0; i < 4; i++) {
		dout << "  " << HEX(group[i]);
	}
	dout << std::endl;
}
