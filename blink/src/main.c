// blinks LD2 (PA5) at 1 Hz with plain register writes, no HAL.
#include "stm32h5xx.h"

// the core runs off HSI after reset, 64 MHz divided by 2
#define CPU_HZ 32000000U

static void delay_ms(uint32_t ms) {
  SysTick->LOAD = CPU_HZ / 1000U - 1U;
  SysTick->VAL = 0U;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
  while (ms--)
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
      ;
  SysTick->CTRL = 0U;
}

int main(void) {
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  // NOTE: the read back makes the write land before GPIOA is touched
  (void)RCC->AHB2ENR;

  GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE5) | GPIO_MODER_MODE5_0;

  for (;;) {
    GPIOA->BSRR = GPIO_BSRR_BS5;
    delay_ms(500);
    GPIOA->BSRR = GPIO_BSRR_BR5;
    delay_ms(500);
  }
}
