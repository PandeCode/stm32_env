#include "usart2.h"

#include "board.h"

void usart2_init(unsigned baud) {
  clock_on(&RCC->APB1LENR, RCC_APB1LENR_USART2EN);
  gpio_alternate(GPIOA, 2, 7);
  gpio_alternate(GPIOA, 3, 7);
  // the kernel clock is pclk1 after reset, 16x oversampling
  USART2->BRR = (CPU_HZ + baud / 2U) / baud;
  USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void usart2_putc(char c) {
  while (!(USART2->ISR & USART_ISR_TXE_TXFNF))
    ;
  USART2->TDR = (uint8_t)c;
}

char usart2_getc(void) {
  // an overrun stops reception until it is cleared
  if (USART2->ISR & USART_ISR_ORE)
    USART2->ICR = USART_ICR_ORECF;
  while (!(USART2->ISR & USART_ISR_RXNE_RXFNE))
    ;
  return (char)USART2->RDR;
}
