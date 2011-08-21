#ifndef _SCALE_THERM_H
#define _SCALE_THERM_H

#include "tlm/arissat/ss_types_common.h"

// Convert an adc10 reading to a temperature in celsius
ss_temp_t scale_thermistor_C(const ss_adc10_t val);

#endif // _SCALE_THERM_H
