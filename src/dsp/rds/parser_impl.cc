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

using namespace gr::rds;

parser::sptr
parser::make(bool log, bool debug) {
  return gnuradio::get_initial_sptr(new parser_impl(log, debug));
}

parser_impl::parser_impl(bool log, bool debug)
	: gr::block ("gr_rds_parser",
			gr::io_signature::make (0, 0, 0),
			gr::io_signature::make (0, 0, 0)),
	log(log),
	debug(debug)
{
	message_port_register_in(pmt::mp("in"));
	set_msg_handler(pmt::mp("in"), boost::bind(&parser_impl::parse, this, _1));
	message_port_register_out(pmt::mp("out"));
	reset();
}

parser_impl::~parser_impl() {
}

void parser_impl::reset() {
	gr::thread::scoped_lock lock(d_mutex);

	memset(radiotext, ' ', sizeof(radiotext));
	memset(program_service_name, '.', sizeof(program_service_name));

    program_identification         = 0;
	radiotext_AB_flag              = 0;
	traffic_program                = false;
	traffic_announcement           = false;
	music_speech                   = false;
	program_type                   = 0;
	pi_country_identification      = 0;
	pi_area_coverage               = 0;
	pi_program_reference_number    = 0;
	mono_stereo                    = false;
	artificial_head                = false;
	compressed                     = false;
	static_pty                     = false;
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
	unsigned int af_code_1 = 0;
	unsigned int af_code_2 = 0;
	unsigned int  no_af    = 0;
	double af_1            = 0;
	double af_2            = 0;
	char flagstring[8]     = "0000000";
	
	traffic_program        = (group[1] >> 10) & 0x01;       // "TP"
	traffic_announcement   = (group[1] >>  4) & 0x01;       // "TA"
	music_speech           = (group[1] >>  3) & 0x01;       // "MuSp"

	bool decoder_control_bit      = (group[1] >> 2) & 0x01; // "DI"
	unsigned char segment_address =  group[1] & 0x03;       // "DI segment"

	program_service_name[segment_address * 2]     = (group[3] >> 8) & 0xff;
	program_service_name[segment_address * 2 + 1] =  group[3]       & 0xff;

	/* see page 41, table 9 of the standard */
	switch (segment_address) {
		case 0:
			mono_stereo=decoder_control_bit;
		break;
		case 1:
			artificial_head=decoder_control_bit;
		break;
		case 2:
			compressed=decoder_control_bit;
		break;
		case 3:
			static_pty=decoder_control_bit;
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
	flagstring[6] = static_pty             ? '1' : '0';
	static std::string af_string;

	if(!B) { // type 0A
		af_code_1 = int(group[2] >> 8) & 0xff;
		af_code_2 = int(group[2])      & 0xff;
		af_1 = decode_af(af_code_1);
		af_2 = decode_af(af_code_2);

		if(af_1) {
			no_af += 1;
		}
		if(af_2) {
			no_af += 2;
		}

		std::string af1_string;
		std::string af2_string;
		/* only AF1 => no_af==1, only AF2 => no_af==2, both AF1 and AF2 => no_af==3 */
		if(no_af) {
			if(af_1 > 80e3) {
				af1_string = str(boost::format("%2.2fMHz") % (af_1/1e3));
			} else if((af_1<2e3)&&(af_1>100)) {
				af1_string = str(boost::format("%ikHz") % int(af_1));
			}
			if(af_2 > 80e3) {
				af2_string = str(boost::format("%2.2fMHz") % (af_2/1e3));
			} else if ((af_2 < 2e3) && (af_2 > 100)) {
				af2_string = str(boost::format("%ikHz") % int(af_2));
			}
		}
		if(no_af == 1) {
			af_string = af1_string;
		} else if(no_af == 2) {
			af_string = af2_string;
		} else if(no_af == 3) {
			af_string = str(boost::format("%s, %s") % af1_string %af2_string);
		}
	}

	lout << "==>" << std::string(program_service_name, 8)
		<< "<== -" << (traffic_program ? "TP" : "  ")
		<< '-' << (traffic_announcement ? "TA" : "  ")
		<< '-' << (music_speech ? "Music" : "Speech")
		<< '-' << (mono_stereo ? "MONO" : "STEREO")
		<< " - AF:" << af_string << std::endl;

	send_message(1, std::string(program_service_name, 8));
	send_message(3, flagstring);
	send_message(6, af_string);
}

double parser_impl::decode_af(unsigned int af_code) {
	static unsigned int number_of_freqs = 0;
	static bool vhf_or_lfmf             = 0; // 0 = vhf, 1 = lf/mf
	double alt_frequency                = 0; // in kHz

	if((af_code == 0) ||                              // not to be used
		( af_code == 205) ||                      // filler code
		((af_code >= 206) && (af_code <= 223)) || // not assigned
		( af_code == 224) ||                      // No AF exists
		( af_code >= 251)) {                      // not assigned
			number_of_freqs = 0;
			alt_frequency   = 0;
	}
	if((af_code >= 225) && (af_code <= 249)) {        // VHF frequencies follow
		number_of_freqs = af_code - 224;
		alt_frequency   = 0;
		vhf_or_lfmf     = 1;
	}
	if(af_code == 250) {                              // an LF/MF frequency follows
		number_of_freqs = 1;
		alt_frequency   = 0;
		vhf_or_lfmf     = 0;
	}

	if((af_code > 0) && (af_code < 205) && vhf_or_lfmf)
		alt_frequency = 100.0 * (af_code + 875);          // VHF (87.6-107.9MHz)
	else if((af_code > 0) && (af_code < 16) && !vhf_or_lfmf)
		alt_frequency = 153.0 + (af_code - 1) * 9;        // LF (153-279kHz)
	else if((af_code > 15) && (af_code < 136) && !vhf_or_lfmf)
		alt_frequency = 531.0 + (af_code - 16) * 9 + 531; // MF (531-1602kHz)

    (void) number_of_freqs;
	return alt_frequency;
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
		lout << boost::format("program item: %id, %i, %i ") % day % hour % minute;
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
		std::memset(radiotext, ' ', sizeof(radiotext));
	}
	radiotext_AB_flag = (group[1] >> 4) & 0x01;

	if(!B) {
		radiotext[text_segment_address_code *4     ] = (group[2] >> 8) & 0xff;
		radiotext[text_segment_address_code * 4 + 1] =  group[2]       & 0xff;
		radiotext[text_segment_address_code * 4 + 2] = (group[3] >> 8) & 0xff;
		radiotext[text_segment_address_code * 4 + 3] =  group[3]       & 0xff;
	} else {
		radiotext[text_segment_address_code * 2    ] = (group[3] >> 8) & 0xff;
		radiotext[text_segment_address_code * 2 + 1] =  group[3]       & 0xff;
	}
	lout << "Radio Text " << (radiotext_AB_flag ? 'B' : 'A')
		<< ": " << std::string(radiotext, sizeof(radiotext))
		<< std::endl;
	send_message(4,std::string(radiotext, sizeof(radiotext)));
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

	std::string time = str(boost::format("%02i.%02i.%4i, %02i:%02i (%+.1fh)")\
		% day % month % (1900 + year) % hours % minutes % local_time_offset);
	lout << "Clocktime: " << time << std::endl;

	send_message(5,time);
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
	bool D = (group[2] > 15) & 0x1; // 1 = diversion recommended
	static unsigned long int free_format[4];
	static int no_groups = 0;

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
		lout << "#user msg# " << (D ? "diversion recommended, " : "");
		if(F) {
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
			label = (free_format[i] && (0xf << ff_pointer));
			content_length = optional_content_lengths[label];
			ff_pointer -= content_length;
			content = (free_format[i] && (int(pow(2, content_length) - 1) << ff_pointer));
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
	static char ps_on[8] = {' ',' ',' ',' ',' ',' ',' ',' '};
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
				lout << boost::format("AF:%3.2fMHz %3.2fMHz") % (af_1/1000) % (af_2/1000);
			break;
			case 5: // mapped frequencies
			case 6: // mapped frequencies
			case 7: // mapped frequencies
			case 8: // mapped frequencies
				af_1 = 100.0 * (((information >> 8) & 0xff) + 875);
				af_2 = 100.0 * ((information & 0xff) + 875);
				lout << boost::format("TN:%3.2fMHz - ON:%3.2fMHz") % (af_1/1000) % (af_2/1000);
			break;
			case 9: // mapped frequencies (AM)
				af_1 = 100.0 * (((information >> 8) & 0xff) + 875);
				af_2 = 9.0 * ((information & 0xff) - 16) + 531;
				lout << boost::format("TN:%3.2fMHz - ON:%ikHz") % (af_1/1000) % int(af_2);
			break;
			case 10: // unallocated
			break;
			case 11: // unallocated
			break;
			case 12: // linkage information
				lout << boost::format("Linkage information: %x%x")
					% ((information >> 8) & 0xff) % (information & 0xff);
			break;
			case 13: // PTY(ON), TA(ON)
				ta_on = information & 0x01;
				pty_on = (information >> 11) & 0x1f;
				lout << "PTY(ON):" << pty_table[int(pty_on)];
				if(ta_on) {
					lout << " - TA";
				}
			break;
			case 14: // PIN(ON)
				lout << boost::format("PIN(ON):%x%x")
					% ((information >> 8) & 0xff) % (information & 0xff);
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

void parser_impl::parse(pmt::pmt_t msg) {
	if(!pmt::is_blob(msg)) {
		dout << "wrong input message (no blob)" << std::endl;
	}
	if(pmt::blob_length(msg) != 4 * sizeof(unsigned long)) {
		dout << "input message has wrong size ("
			<< pmt::blob_length(msg) << ")" << std::endl;
	}
	unsigned int *group = (unsigned int*)pmt::blob_data(msg);

	unsigned int group_type = (unsigned int)((group[1] >> 12) & 0xf);
	bool ab = (group[1] >> 11 ) & 0x1;

	lout << boost::format("%02i%c ") % group_type % (ab ? 'B' :'A');
	lout << "(" << rds_group_acronyms[group_type] << ")";

	program_identification = group[0];     // "PI"
	program_type = (group[1] >> 5) & 0x1f; // "PTY"
	int pi_country_identification = (program_identification >> 12) & 0xf;
	int pi_area_coverage = (program_identification >> 8) & 0xf;
	unsigned char pi_program_reference_number = program_identification & 0xff;
	std::string pistring = str(boost::format("%04X") % program_identification);
	send_message(0, pistring);
	send_message(2, pty_table[program_type]);

	lout << " - PI:" << pistring << " - " << "PTY:" << pty_table[program_type];
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

