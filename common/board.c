#include "board.h"

// systick counts down 24 bits at CPU_HZ, so one call waits at most 0.5 s
static void delay_ticks(uint32_t ticks) {
  SysTick->LOAD = ticks - 1U;
  SysTick->VAL = 0U;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
  while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
    ;
  SysTick->CTRL = 0U;
}

void delay_us(uint32_t us) {
  while (us > 1000U) {
    delay_ticks(CPU_HZ / 1000U);
    us -= 1000U;
  }
  if (us)
    delay_ticks(us * (CPU_HZ / 1000000U));
}

void delay_ms(uint32_t ms) {
  while (ms--)
    delay_ticks(CPU_HZ / 1000U);
}

void fail(void) {
  for (;;)
    ;
}

static void gpio_clock(GPIO_TypeDef *port) {
  // the GPIOxEN bits sit in port order, A at bit 0, ports 0x400 apart
  uint32_t index = ((uint32_t)port - GPIOA_BASE) / 0x400U;
  clock_on(&RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN << index);
}

enum { MODE_INPUT, MODE_OUTPUT, MODE_ALTERNATE, MODE_ANALOG };

static void gpio_mode(GPIO_TypeDef *port, unsigned pin, uint32_t mode) {
  field_write(&port->MODER, 2U, pin * 2U, mode);
}

void gpio_output(GPIO_TypeDef *port, unsigned pin, int high) {
  gpio_clock(port);
  // level first so the pin never glitches
  gpio_write(port, pin, high);
  gpio_mode(port, pin, MODE_OUTPUT);
}

void gpio_open_drain(GPIO_TypeDef *port, unsigned pin) {
  gpio_clock(port);
  port->OTYPER |= 1U << pin;
  // 01 is pull up
  field_write(&port->PUPDR, 2U, pin * 2U, 1U);
}

void gpio_alternate(GPIO_TypeDef *port, unsigned pin, unsigned af) {
  gpio_clock(port);
  field_write(&port->AFR[pin / 8U], 4U, (pin % 8U) * 4U, af);
  gpio_mode(port, pin, MODE_ALTERNATE);
}
