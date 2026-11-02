// i2c1 master on PB6 (scl) and PB7 (sda) at 100 kHz, the arduino scl/sda pins
#ifndef I2C1_H
#define I2C1_H

#include <stddef.h>
#include <stdint.h>

void i2c1_init(void);
// whole transfers to a 7 bit address, 0 on success, -1 on a nack. at most
// 255 bytes each
int i2c1_write(uint8_t addr, const uint8_t *buf, size_t len);
int i2c1_read(uint8_t addr, uint8_t *buf, size_t len);

#endif
