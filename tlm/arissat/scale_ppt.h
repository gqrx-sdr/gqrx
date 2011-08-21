#ifndef _SCALE_PPT_H
#define _SCALE_PPT_H

// Compute PPT Solar voltage from the 8 bit ADC value
double scale_ppt_sp_voltage(u8 val);

// Compute the PPT PWM Setpoint current from the 8 bit CCP pulse width
double scale_ppt_pwm_setpoint(u8 val);

// Compute the PPT Solar current from the 10 bit ADC value
double scale_ppt_sp_current(ss_adc10_t val);

#endif // _SCALE_PPT_H
