#include "i2c1.h"

#include "board.h"

void i2c1_init(void) {
  clock_on(&RCC->APB1LENR, RCC_APB1LENR_I2C1EN);
  gpio_open_drain(GPIOB, 6);
  gpio_open_drain(GPIOB, 7);
  gpio_alternate(GPIOB, 6, 4);
  gpio_alternate(GPIOB, 7, 4);

  // 100 kHz: rm0481's 8 MHz timings, prescaler 8 for the 32 MHz pclk1
  I2C1->TIMINGR =
      (7U << I2C_TIMINGR_PRESC_Pos) | (4U << I2C_TIMINGR_SCLDEL_Pos) |
      (2U << I2C_TIMINGR_SDADEL_Pos) | (0x0FU << I2C_TIMINGR_SCLH_Pos) |
      (0x13U << I2C_TIMINGR_SCLL_Pos);
  I2C1->CR1 = I2C_CR1_PE;
}

static void start(uint8_t addr, size_t len, uint32_t read) {
  // autoend sends the stop after the last byte, or right after a nack
  I2C1->CR2 = (uint32_t)addr << 1 | len << I2C_CR2_NBYTES_Pos | read |
              I2C_CR2_AUTOEND | I2C_CR2_START;
}

static int finish(void) {
  while (!(I2C1->ISR & I2C_ISR_STOPF))
    ;
  int nack = I2C1->ISR & I2C_ISR_NACKF;
  // a nack can leave the next byte in txdr, the next write would send it
  I2C1->ISR = I2C_ISR_TXE;
  I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
  return nack ? -1 : 0;
}

int i2c1_write(uint8_t addr, const uint8_t *buf, size_t len) {
  start(addr, len, 0U);
  for (size_t i = 0; i < len; i++) {
    while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)))
      ;
    if (I2C1->ISR & I2C_ISR_NACKF)
      break;
    I2C1->TXDR = buf[i];
  }
  return finish();
}

int i2c1_read(uint8_t addr, uint8_t *buf, size_t len) {
  start(addr, len, I2C_CR2_RD_WRN);
  for (size_t i = 0; i < len; i++) {
    while (!(I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF)))
      ;
    if (I2C1->ISR & I2C_ISR_NACKF)
      break;
    buf[i] = (uint8_t)I2C1->RXDR;
  }
  return finish();
}
