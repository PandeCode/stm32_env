// adc1 on the chip's temperature sensor and its reference voltage
#ifndef ADC1_H
#define ADC1_H

#include <stdint.h>

#define ADC1_TEMP 16U
#define ADC1_VREFINT 17U

// powers up, calibrates and turns on adc1 with both sensors. every channel
// samples for 640.5 clocks, the sensors want the long ones
void adc1_init(void);
// one 12 bit conversion
uint16_t adc1_read(unsigned ch);
// from st's factory readings, all taken at 3.3 V: vrefint gives vdda, and
// the sensor scaled to 3.3 V gives the temperature
unsigned adc1_vdda_mv(uint16_t vrefint);
int adc1_temp_c(uint16_t ts, unsigned vdda_mv);

#endif
