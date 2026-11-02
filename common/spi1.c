#include "spi1.h"

#include "board.h"

void spi1_init(void) {
  // NOTE: the kernel clock defaults to pll1_q, which is off after reset.
  // per_ck is HSI, so the bus runs at 32 MHz / 8
  RCC->CCIPR3 = (RCC->CCIPR3 & ~RCC_CCIPR3_SPI1SEL) | RCC_CCIPR3_SPI1SEL_2;
  clock_on(&RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
  gpio_alternate(GPIOA, 5, 5);
  gpio_alternate(GPIOA, 7, 5);
  // 4 MHz sck is too fast for the low reset speed
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED5_1 | GPIO_OSPEEDR_OSPEED7_1;

  // software slave select held high before master goes on. the other way
  // round the low select faults it (MODF) and it drops back to slave
  SPI1->CR1 = SPI_CR1_SSI;
  // crcsize has to match dsize even with crc off
  SPI1->CFG1 = (2U << SPI_CFG1_MBR_Pos) | (7U << SPI_CFG1_CRCSIZE_Pos) |
               (7U << SPI_CFG1_DSIZE_Pos);
  // afcntr keeps the pins driven while spi is off between writes
  SPI1->CFG2 =
      SPI_CFG2_AFCNTR | SPI_CFG2_SSM | SPI_CFG2_MASTER | SPI_CFG2_COMM_0;
}

void spi1_write(const uint8_t *buf, size_t len) {
  while (len) {
    // a sized transfer, so eot waits for the last bit. txc can read 1 before
    // the first one leaves. tsize only takes with spi off, and it goes off
    // here, not after eot, where it can cut the last clock (es0621 2.17.2)
    size_t n = len < 0xFFFFU ? len : 0xFFFFU;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR2 = n;
    SPI1->CR1 |= SPI_CR1_SPE;
    SPI1->CR1 |= SPI_CR1_CSTART;
    for (size_t i = 0; i < n; i++) {
      while (!(SPI1->SR & SPI_SR_TXP))
        ;
      // NOTE: a byte store puts one frame in the fifo, a word store packs four
      *(volatile uint8_t *)&SPI1->TXDR = buf[i];
    }
    while (!(SPI1->SR & SPI_SR_EOT))
      ;
    SPI1->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
    buf += n;
    len -= n;
  }
}
