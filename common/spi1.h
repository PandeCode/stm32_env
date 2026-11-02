// spi1 as a transmit only master on PA5 (sck) and PA7 (mosi), mode 0
#ifndef SPI1_H
#define SPI1_H

#include <stddef.h>
#include <stdint.h>

void spi1_init(void);
// returns once the last bit is out on the wire
void spi1_write(const uint8_t *buf, size_t len);

#endif
