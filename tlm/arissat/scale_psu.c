#include "tlm/arissat/ss_types_common.h"
#include "tlm/arissat/scale_psu.h"

// Compute PSU VDD from the 2.5V vref
double scale_psu_vdd(ss_adc10_t vref_2p5)
{
	// ADC = 2.5V * (1024/supply)
	return ( 2.500*1023.0/vref_2p5);
}

// Scale the 28V bus to a human readable value
double scale_psu_v_batt(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	double ratio = (4.7)/(4.7+33);

	return (scale_psu_vdd(vref_2p5)*val)/1023.0/ratio;
}

// Scale the Battery Current to a human readable value
double scale_psu_i_batt(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*(((s16) val)- ((s16)vref_2p5)))/1023.0/(0.020*60.0);
}

// Scale the battery net charge to a human readable value
// Returns the value in Coulombs (amp-seconds)
double scale_psu_c_net_batt_s64(s64 val, ss_adc10_t vref_2p5)
{
	return scale_psu_vdd(vref_2p5)*(val/1023.0/(0.020*60.0)/PSU_CHRG_SAMPLES_PER_SEC);
}

// Scale the battery charge to a human readable value
// Returns the value in Coulombs (amp-seconds)
double scale_psu_c_chg_batt_u64(u64 val, ss_adc10_t vref_2p5)
{
	return scale_psu_vdd(vref_2p5)*(val/1023.0/(0.020*60.0)/PSU_CHRG_SAMPLES_PER_SEC);
}

// Scale the battery discharge to a human readable value
double scale_psu_c_dischg_batt_u64(u64 val, ss_adc10_t vref_2p5 )
{
	return scale_psu_vdd(vref_2p5)*(val/1023.0/(0.020*60.0)/PSU_CHRG_SAMPLES_PER_SEC);
}


// Scale the 5V current to a human readable value
double scale_psu_i_5v(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.1*50);
}

// Scale the 8V current to a human readable value
double scale_psu_i_8v(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.1*50);
}

// Scale the IHU current to a human readable value
double scale_psu_i_ihu(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.1*50);
}

// Scale the SDX current to a human readable value
double scale_psu_i_sdx(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.1*50);
}

// Scale the Experiment current to a human readable value
double scale_psu_i_experiment(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.332*50);
}

// Scale the Camera current to a human readable value
double scale_psu_i_camera(ss_adc10_t val, ss_adc10_t vref_2p5)
{
	return (scale_psu_vdd(vref_2p5)*val)/1023.0/(0.1*50);
}

