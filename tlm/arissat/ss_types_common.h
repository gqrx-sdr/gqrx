/*
 *  ss_types_common.h
 *  suitsat_os
 *
 *  Created by N9WXU on 3/16/08.
 *  Copyright 2008 AMSAT. All rights reserved.
 *
 */

#ifndef _SS_TYPES_COMMON_H_
#define _SS_TYPES_COMMON_H_

// True is not TRUE because GenericTypeDefs.h has TRUE defined in a BOOL enum
// same with False
#define True 1
#define False 0

// Call out the basic defined types, defined to be the same across multiple compilers
#include "tlm/arissat/ss_stdint.h"

#define PERCENT_100 (100*2)
#define PERCENT_75  (75*2)
#define PERCENT_50  (50*2)
#define PERCENT_25  (25*2)
#define PERCENT_0   0

#define MINUTE   60
#define HOUR     (60 * MINUTE)

// BPSK Modes
#define BPSK400  	0
#define BPSK1000	1

// Number of available PPTs
#define PPT_COUNT				6
// Number of utilized Experiments
#define SYS_EXPERIMENT_COUNT		1


typedef s8   ss_temp_t;        // 1 byte -128 to 127 C
typedef s16  ss_amp_t;         // 2 byte -4096.00 to 4096.00 mA (125uA/lsb)
typedef s16  ss_volt_t;        // 2 byte  -327.68 to 327.67V
typedef u32  ss_time_t;        // 4 bytes 0-lots seconds
typedef u8   ss_percent_t;     // 1 byte  0-127.5%

typedef u16  ss_adc10_t;       // 2 bytes - 10bit ADC result

// How often the PSU samples the current for total charge
#define PSU_CHRG_SAMPLES_PER_SEC	50

// Battery data
typedef struct PACKED {
	ss_adc10_t	v_raw;					// Raw ADC value on voltage
	ss_adc10_t	i_raw;					// Raw ADC value on bidir current.  Above 2.5V ref indicates charging
	ss_adc10_t	ref2p5_raw;				// 2.5V center current reference

    s48 	 	c_battery_net_raw;      // flight total net coulombs moved into or out of the battery
} PACKED ss_battery_status_t;

// Break these up as to not jam up the I2C bus or make the packet too big to the PSU
typedef struct PACKED
{
	u48			c_battery_chrg_raw;		// Flight total charging coulombs (in to battery)
	u48			c_battery_dischrg_raw;	// Flight total discharging coulombs (out of battery)
} ss_battery_history_t;


//
//    Mission Modes enumeration
//    EMERGENCY_PWR  RF very low duty cycle
//    LOW_PWR        RF low duty cycle (30sec on 120 sec off)
//    HIGH_PWR       RF on all the time
//	TXINHIBIT_PWR  RF off all the time
typedef enum {EMERGENCY_PWR, LOW_PWR, HIGH_PWR, TXINHIBIT_PWR} ss_mission_modes_t;

/**********************************************************/
/* SDX-IHU communications data structures                 */
/**********************************************************/

typedef struct PACKED {			//**** Updated 6/17/2010 AA2TX *****
	u16	header;			// This is the software version
	u8	frameNumber;         	// Tracks frame
	u8 	Reserved[63];		// Future use
	u8	checksum;            	// packet checksum - sum of all bytes
    	u8	zero;                	// zero pad to put SDO low
} ss_SDXIHUCommPacket_t; 		// 68 bytes

typedef struct  PACKED
{
    u8 FM;
    u8 CW;
    u8 BPSK;
    u8 TRANSPONDER;
} ss_powerLevel_t;

typedef struct PACKED
{
	u16 header;             /* Could be the software version        */
	u8 frameNumber;         /* Tracks the frame                     */
	ss_powerLevel_t  powerLevelCmd;     /* Sets the SDX power level             */
	u8 CW;                  /* CW Output                            */
	u8 telemData;           /* This is the telemtry data            */
	u8 sdx_debug;			/* debug character to SDX */
	u8 BPSKMode;			/* Use BPSK mode definitions            */
	u8 MissionMode;			/* Use Mission mode enumerations		*/
	u8 reserved[6];         /* For future use                       */
	u8 FMdata[48];          /* 48 samples of FM Data                */
    u8 checksum;            /* packet checksum (sum of all bytes)   */ // AA2TX changed to char
                                                                       // N9WXU changed to u8 (equiv.)
    u8 zero;                /* zero pad to put SDO low   */
} ss_IHUSDXCommPacket_t; 	// 68 bytes

typedef enum {ON=0x2E, OFF=0x55} ss_power_state_t;

/********************************************************************/
/* PSU Data Structures                                              */
/********************************************************************/

typedef union PACKED {
	struct PACKED 
	{
		unsigned 	por:1;					// POR Flag
		unsigned 	bor:1;					// BOR Flag
		unsigned 	to:1;					// TO Flag
		unsigned 	pd:1;					// PD Flag

		unsigned	forced_reset:1;			// Forced reset flag
		unsigned	rollover_reset:1;		// Rollover reset flag

		unsigned 	unused:2;				// Unused
	};
	u8 data[1];
} ss_psu_reset_reason_t;


typedef union  PACKED
{
    struct PACKED
    {
		
		u8 osc_status;						// Oscillator status
		u8 xtal_failed_cnt;
		u8 pin_disturbs;					// Flags if we have change our pin settings because they got changes

#if 0
		// This would be useful, if we had Non-volatile storage implemented fully
		// And enough usable RAM on the micro
        u8 commanded_ihu_resets;

        u8 psu_bor_resets;
        u8 psu_por_resets;
        u8 psu_wdt_resets;

        u8 psu_i2c_cmd_fail_resets;
#endif

		// Self check errors, that we may or may not be able to see
		// Storing these into EEPROM would be great, but more work
		u8 ihu_frc_resets;						// Number of times the IHU has been forced to be reset
	    u8 sdx_auto_turn_ons;					// Number of times the SDX has been automatically turned on.
		u8 ihu_recoveries;						// Number of times we thought the IHU was dead but it came back
		u8 low_batt_camera_shutdowns;			// Number of times we shut down the cameras because of low batteries

		// I2C debugging info
		unsigned int i2c_wcol_errors:4;
		unsigned int i2c_overflow_errors:4;

// 		Current I2C driver doesn't have illegal states, so we don't need to log it.
//		u8 i2c_illegal_sspstat;
//		u8 i2c_illegal_sspstat_count;		// Number of times it's occurred

		// Software status tracking
		u8 i2c_bad_msg_cnt;						// Bad I2C messages that are not formatted correctly
		u8 i2c_bad_state;						// Message register in process when reset
		u8 i2c_bus_idle_resets;					// Number of times the I2C bus is reset due to lack of activity
		u8 i2c_bus_hang_resets;					// Number of times the I2C bus is reset due to an stuck message

		u8 i2c_bus_reset_sspcon;				// SSPCON just before the bus is reset
		u8 i2c_bus_reset_sspstat;				// SSPSTAT just before the bus is reset
		u8 i2c_bus_reset_state;					// State I2C bus was in when we reset it
		u8 i2c_bus_reset_xmt_size;				// Associated data with the state at reset
		u8 i2c_bus_reset_act_size;				// Associated data with the state at reset

		u8 dbl_cmd_first_second_mismatch;		// Commands that need to be repeated to exceute that were not correct
		u8 ppt_poll_busy;						// We're still polling the PPT when it's time to start the next one

		// Reset info
		ss_psu_reset_reason_t reset_reason;		// The reset reason(s)
        u8 psu_suicide_attempts;				// The number of times the PSU has intentionally reset itself
    };
    u8 data[21];
} ss_psu_status_t;

typedef enum {DEVICE_OFF=0, DEVICE_COMMAND_ON=1, DEVICE_FAULT=2, DEVICE_ACTIVE=3} ss_psu_device_state_t;

typedef union PACKED {
	struct PACKED 
	{
		u48			c_total_raw;			// Total Coulombs value (raw)
		u8 			i_raw_lsb;				// Raw current value
		unsigned 	i_raw_msb:4;

		unsigned 	status:2;				// Device status
	};
	u8 data[8];
} ss_power_info_t;

typedef union PACKED {
	struct PACKED 
	{
		u48			c_total_raw;			// Total Coulombs value (raw)
		u8 			i_raw_lsb;				// Raw current value
		unsigned 	i_raw_msb:4;

		// actually ss_psu_device_state_t
		unsigned 	status1:2;				// Camera 1 status
		unsigned 	status2:2;				// Camera 2 status
		unsigned 	status3:2;				// Camera 3 status
		unsigned 	status4:2;				// Camera 4 status
	};
	u8 data[9];
} ss_camera_power_info_t;

typedef union PACKED
{
     struct PACKED
    {
        unsigned status:2;				// actually ss_psu_device_state_t
        unsigned :6;
    };
    u8 data;
} ss_power_status_t;

typedef union PACKED
{
    struct PACKED
    {
        unsigned unit1:2;
        unsigned unit2:2;
        unsigned unit3:2;
        unsigned unit4:2;
    };
    u8 data;    
} ss_multi_status_t;

// Status for one individual PPT, as processed by the PSU
typedef union  PACKED
{
    struct PACKED
    {
        u48 			sp_energy_osc_raw;		// Solar panel Energy (since last PSU reboot)

        u8				sp_temp_raw_lsb;		// Raw ADC reading for Solar Temperature
		u8				sp_temp_raw_msb_diode_temp_raw_msb;		// SP Temp in upper nibble, diode temp in lower nibble
        u8		 		diode_temp_raw_lsb;		// Raw ADC reading for PPT Diode Temperature

        u8 				ind_temp_raw_lsb;		// Raw ADC reading for PPT Inductor Temperature
		u8				ind_temp_raw_msb_fet_temp_raw_msb;		// Ind in upper nibble, fet temp in lower nibble
        u8 				fet_temp_raw_lsb;		// Raw ADC reading for PPT FET Temperature

		ss_adc10_t		sp_current_adc_raw;		// Raw ADC reading for Instantaneous Solar Panel current 

		u8 				sp_voltage_raw;			// Solar Panel voltage
		u8				osc_ccp_current_setpt;	// Solar panel current PWM setpoint

		u8				aged;					// How old this data is;  zero means it's fresh, otherwise it's
		u8 				corrupt;				// Count of corrupted packets received from this PPT
    } ;
    u8 data[18];
} ss_psu_ppt_status_t;



typedef struct  PACKED
{
	ss_battery_status_t		batt_status;	// Battery status and overall values
	ss_battery_history_t	batt_history;	// Battery Charge/Discharge history

	ss_camera_power_info_t	camera;			// Camera power info
	ss_power_info_t			experiment;		// Experiment Power info

	ss_power_info_t			ihu;			// IHU Power info
	ss_power_info_t			sdx;			// SDX Power info

	ss_power_info_t			ps5v;			// 5V Power Supply info
	ss_power_info_t			ps8v;			// 8V Power Supply info

	ss_power_status_t		rf;				// RF Power on/off
	ss_power_status_t		spare;			// Spare Power on/off
} ss_power_t;

// Temperatures measured on the IHU
typedef struct  PACKED
{
	// Temperatures measured thru the 50pin cable
	ss_adc10_t	rf;							// RF Module Internal Temperature
	ss_adc10_t	control_panel;				// Control Panel Internal Temperature

	// Temperatures measured thru the temp sensor cable
	ss_adc10_t	experiment;					// Experiment Temperature
	ss_adc10_t	bottom_cam;					// Bottom Cameras
	ss_adc10_t	top_cam;					// Top Cameras
	ss_adc10_t	ihu_enclosure;				// IHU Box

	// Temperature measured thru the battery cable
	ss_adc10_t	battery;					// Battery

	// PCBs inside the IHU Box
	ss_adc10_t	psu_pcb;
	ss_adc10_t	ihu_pcb;

} ss_ihu_temps_t;			// All temperatures measured from the IHU

/**********************************************************/
/* IHU Telemetry data structures                          */
/**********************************************************/
#define IHU_I2C_BUS_COUNT			2		/* Two I2C busses present */

typedef struct PACKED
{	
	u8      bad_state;					// Illegal/unknown state detected
	u8		bad_nack;					// Unexpected NACKs
	u8		timeouts;					// Bus timeouts
} ss_ihu_i2c_stats_t;

// IHU statistics about the Command decoder 
typedef struct PACKED
{
	u8 		rcv_too_big;				// Received too big
	u8		rcv_ovrrn_errors;			// overrun errors
	u8		rcv_misc_errors;			// Received Misc UART errors (Framing)
	u8		rcv_chars;					// Received characters
	u8		rcv_cmds;					// Received commands
	u8		exec_cmds;					// Executed commands
	u8		unknown_cmds;				// Unknown commands received from Command Decoder
} ss_ihu_cmddec_stats_t;

typedef struct PACKED
{
	u8 		uart_rcv_errors;			// Received errors (overrun/Framing)
	u8		state;						// Current state
	u8      bad_state;					// Illegal/unknown state detected
	u8		full_runs;					// Number of times the experiment has been run fully
	u16		quick_runs;					// Number of times the experiment has been run just to get data
	u16		aborted_pwr;				// Was running but aborted for power reasons
	u32		quick_timer;				// Timer for next experiment event
	u32		full_timer;					// Timer for next experiment full run
} ss_ihu_exp_stats_t;

typedef struct PACKED
{
	u8		i2c_timeout_failed;			// I2C Timeout failed
} ss_ihu_psu_stats_t;

typedef struct PACKED
{
	u8		frc_resets;					// Number of times the SDX had to be reset (power cycled)
	u8		recoveries;					// Number of times the SDX started working after reset
	u8		bad_chksum;					// Packets received from SDX with a bad checkum
	u8		bad_header;					// Packets received from SDX with a bad header
	u8		bad_frame_seq;				// Packets received from SDX with an unexpected frame sequence number
} ss_ihu_sdx_stats_t;

// Power mode statistics
typedef struct PACKED
{
	u16			force_hp_timer;			// Timer for how long to force High Power Mode
	u16			camera_timer;			// How long the cameras have been on
	u16			hi_enable_timer;		// How long to enable high power mode
	u16			hi_lockout_timer;		// How long to run stay out of high power mode
	u16			psu_fail_timer;			// How long until we declare the PSU failed
	
	ss_volt_t	min_batt_volts;			// Minimum Battery voltage seen
	ss_time_t	min_batt_MET;			// MET of Minimum Battery voltage


	unsigned	cameras_active:1;		// Cameras Active Flag
	unsigned	battery_good:1;			// Battery good flag
} ss_ihu_power_stats_t;			

typedef struct PACKED
{
	u32 	rcon;						// Reset reason from RCON
	u32		ErrorEPC;					// Error PC
} ss_ihu_reset_stats_t;

typedef struct PACKED
{
	u16		psu_delayed_resp;			// PSU Request was still pending when we went to process it
	u16		psu_err_resp;				// PSU Request had an error when we went to process it
	s8		psu_last_err;				// Last errored PSU request results
	u16		psu_stale_secs;				// How stale the PSU data is in seconds. Flags aging if any of the PSU requests fail
} ss_ihu_telem_stats_t;

typedef struct PACKED
{
	u16		sent_rom_ids;				// Number of ROM ID sent
	u16		sent_sd_ids;				// Number of SD IDs sent
} ss_ihu_cw_stats_t;

#define CAMERA_COUNT			4		/* 4 Cameras */

typedef struct PACKED
{
	u16		img_good[CAMERA_COUNT];		// Image captures, good images
	u16		img_rej_brt[CAMERA_COUNT];	// Images rejected for brightness
	u8		img_no_sync[CAMERA_COUNT];	// Image captures attempted, but no sync
	u8		img_no_capt[CAMERA_COUNT];	// Image captures attemped but failed

	u8		i2c_timeout_failed;			// Number of times the I2C driver timeout failed 
} ss_ihu_video_stats_t;

typedef struct PACKED 
{
	u32		osc_status;					// Oscillator status
	u8		osc_failed;					// Flag if the IHU EC oscillator has failed

	u8		core_timer_missed_ticks;	// Number of missed ticks in the core timer


	ss_ihu_i2c_stats_t  	i2c[IHU_I2C_BUS_COUNT];		// I2C Bus status
 	ss_ihu_cmddec_stats_t	cmddec;						// Command Decoder stats
	ss_ihu_exp_stats_t		experiment;					// Experiment statistics
	ss_ihu_psu_stats_t		psu;						// PSU statistics
 	ss_ihu_sdx_stats_t		sdx;						// SDX statistics

	ss_ihu_power_stats_t	power;						// Power statistics
	ss_ihu_reset_stats_t	reset;						// Reset information
	ss_ihu_telem_stats_t	telem;						// Telemetry information
	ss_ihu_cw_stats_t		cw;							// CW statistics
	ss_ihu_video_stats_t	video;						// Video statistics
} ss_ihu_status_t;

// Calculated Telemetry data - values changed from raw counts to human
// readable formats
typedef struct PACKED
{
	ss_volt_t	batt_volts;				// Battery voltage
	ss_amp_t 	batt_curr;				// Battery current
	ss_amp_t	rf_curr;				// RF (8V) current
	ss_temp_t	ihu_encl_temp_c;		// IHU enclosure temperature (degrees c)
    ss_temp_t   control_panel_temp_c;	// Control Panel Temperature (Degrees c)
} ss_calc_telem_t;

/**********************************************************/
/* Telmetry data structures                               */
/**********************************************************/
typedef struct  PACKED
{
    ss_time_t               mission_time;
    ss_mission_modes_t      mission_mode;

	ss_ihu_temps_t			ihu_temps;			// Temperatures measured from the IHU

    ss_power_t              power;

    ss_psu_status_t         psu_status;
	ss_psu_ppt_status_t 	ppt_status[PPT_COUNT];
	ss_ihu_status_t			ihu_status;

	// Calculated telemetry values
	ss_calc_telem_t			calc;
} ss_telem_t;

#endif

