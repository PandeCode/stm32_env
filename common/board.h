// the nucleo-h533re as the programs use it: clock, delays, pins
#ifndef BOARD_H
#define BOARD_H

#include "stm32h5xx.h"

// the core runs off HSI after reset, 64 MHz divided by 2, and so do the
// apb buses
#define CPU_HZ 32000000U

void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

// hangs, `b fail` in gdb stops there
[[noreturn]] void fail(void);

// NOTE: the read back makes the write land before the peripheral is touched
static inline void clock_on(volatile uint32_t *enr, uint32_t bit) {
  *enr |= bit;
  (void)*enr;
}

// these turn the port's clock on first
void gpio_output(GPIO_TypeDef *port, unsigned pin, int high);
// with the weak internal pull up, the nucleo has no i2c pull ups of its own.
// call it before gpio_alternate so the pin never drives high
void gpio_open_drain(GPIO_TypeDef *port, unsigned pin);
void gpio_alternate(GPIO_TypeDef *port, unsigned pin, unsigned af);

// sets a width bit field at pos to val, leaving the rest of the register
static inline void field_write(volatile uint32_t *reg, unsigned width,
                               unsigned pos, uint32_t val) {
  uint32_t mask = ((1U << width) - 1U) << pos;
  *reg = (*reg & ~mask) | ((val << pos) & mask);
}

// LD2, the green led, also D13
#define LD2 GPIOA, 5

static inline void gpio_write(GPIO_TypeDef *port, unsigned pin, int high) {
  port->BSRR = high ? 1U << pin : 1U << (pin + 16U);
}

static inline void gpio_toggle(GPIO_TypeDef *port, unsigned pin) {
  port->ODR ^= 1U << pin;
}

#endif
