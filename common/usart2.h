// usart2 on PA2/PA3, wired to the st-link's virtual com port
#ifndef USART2_H
#define USART2_H

void usart2_init(unsigned baud);
void usart2_putc(char c);
char usart2_getc(void);

#endif
