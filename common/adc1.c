#include "adc1.h"

#include "board.h"

// factory readings at 3.3 V: vrefint at 30 C, the sensor at 30 C and 130 C
#define VREFINT_CAL (*(const volatile uint16_t *)0x08FFF810U)
#define TS_CAL1 (*(const volatile uint16_t *)0x08FFF814U)
#define TS_CAL2 (*(const volatile uint16_t *)0x08FFF818U)

// smp 7, 640.5 clocks, for all ten channels in each register
#define SMP_ALL 0x3FFFFFFFU

void adc1_init(void) {
  // the kernel clock is hclk after reset, 32 MHz
  clock_on(&RCC->AHB2ENR, RCC_AHB2ENR_ADCEN);
  ADC12_COMMON->CCR |= ADC_CCR_TSEN | ADC_CCR_VREFEN;

  // out of deep power down, then the regulator. 25 us covers it (5 us) and
  // the temperature sensor's buffer
  ADC1->CR = 0;
  ADC1->CR = ADC_CR_ADVREGEN;
  delay_us(25);

  // adcaldif 0, single ended
  ADC1->CR |= ADC_CR_ADCAL;
  while (ADC1->CR & ADC_CR_ADCAL)
    ;
  // aden doesn't take for 4 adc clocks after that
  delay_us(1);

  ADC1->SMPR1 = SMP_ALL;
  ADC1->SMPR2 = SMP_ALL;
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  while (!(ADC1->ISR & ADC_ISR_ADRDY))
    ;
}

uint16_t adc1_read(unsigned ch) {
  ADC1->SQR1 = ch << ADC_SQR1_SQ1_Pos;
  ADC1->ISR = ADC_ISR_EOC;
  ADC1->CR |= ADC_CR_ADSTART;
  while (!(ADC1->ISR & ADC_ISR_EOC))
    ;
  return ADC1->DR;
}

unsigned adc1_vdda_mv(uint16_t vrefint) {
  return 3300U * VREFINT_CAL / vrefint;
}

int adc1_temp_c(uint16_t ts, unsigned vdda_mv) {
  int at_3v3 = (int)(ts * vdda_mv / 3300U);
  return 30 + (at_3v3 - TS_CAL1) * 100 / (TS_CAL2 - TS_CAL1);
}
