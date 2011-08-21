#include "tlm/arissat/ss_types_common.h"
#include "tlm/arissat/scale_ppt.h"

// Compute PPT Solar voltage from the 8 bit ADC value
double scale_ppt_sp_voltage(u8 val)
{
	double ratio = 15.0/(15.0+182.0);

	return 5.00*val/255.0/ratio;
}

// Compute the PPT Setpoint current from the 10 bit ADC value
double scale_ppt_pwm_setpoint(u8 val)
{
	double ratio = 10 * 0.5;

	return 5.00*val/253.0/2.0/ratio;

}

// Compute the PPT Solar current from the 10 bit ADC value
double scale_ppt_sp_current(ss_adc10_t val)
{
	double ratio = 10 * 0.5;

	return 5.00*val/1023.0/ratio;
}

