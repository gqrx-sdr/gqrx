#ifndef _SCALE_PSU_H
#define _SCALE_PSU_H

#include "tlm/arissat/ss_stdint.h"

// All voltages returned in Volts
// All currents returned in Amps

// Compute PSU VDD from the 2.5V vref
double scale_psu_vdd(ss_adc10_t vref_2p5);

// Scale the 28V bus to a human readable value
double scale_psu_v_batt(ss_adc10_t val, ss_adc10_t vref_2p5);

// Scale the Battery Current to a human readable value
double scale_psu_i_batt(ss_adc10_t val, ss_adc10_t vref_2p5);

// Scale the battery net charge to Coulombs (amp-seconds)
double scale_psu_c_net_batt_s64(s64 val, ss_adc10_t vref_2p5);
#define scale_psu_c_net_batt_s48(val, vref)  scale_psu_c_net_batt_s64(S48TOS64(val), vref)

// Scale the battery charge to Coulombs (amp-seconds)
double scale_psu_c_chg_batt_u64(u64 val, ss_adc10_t vref_2p5);
#define scale_psu_c_chg_batt_u48(val, vref)  scale_psu_c_chg_batt_u64(U48TOU64(val), vref)

// Scale the battery discharge to Coulombs (amp-seconds)
// Scale the battery discharge to a human readable value
double scale_psu_c_dischg_batt_u64(u64 val, ss_adc10_t vref_2p5);
#define scale_psu_c_dischg_batt_u48(val, vref)	scale_psu_c_dischg_batt_u64(U48TOU64(val), vref)

// Scale the 5V/8V/IHU/SDX/experiment current to a human readable value
double scale_psu_i_5v(ss_adc10_t val, ss_adc10_t vref_2p5);
double scale_psu_i_8v(ss_adc10_t val, ss_adc10_t vref_2p5);
double scale_psu_i_ihu(ss_adc10_t val, ss_adc10_t vref_2p5);
double scale_psu_i_sdx(ss_adc10_t val, ss_adc10_t vref_2p5);
double scale_psu_i_experiment(ss_adc10_t val, ss_adc10_t vref_2p5);

// Scale the Camera current to a human readable value
double scale_psu_i_camera(ss_adc10_t val, ss_adc10_t vref_2p5);

#endif // _SCALE_PSU_H
