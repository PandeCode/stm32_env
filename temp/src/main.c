// the chip's temperature and vdda on the com port twice a second (`just
// serial`). a finger on the chip warms it a degree or two
#include <stdio.h>

#include "adc1.h"
#include "board.h"
#include "usart2.h"

int main(void) {
  usart2_init(115200);
  adc1_init();
  for (;;) {
    unsigned vdda = adc1_vdda_mv(adc1_read(ADC1_VREFINT));
    int temp = adc1_temp_c(adc1_read(ADC1_TEMP), vdda);
    printf("temp %d C  vdda %u mV\n", temp, vdda);
    delay_ms(500);
  }
}
