// adafruit seesaw (the samd09 helper chip on their shields) over i2c.
// #define SEESAW_IMPLEMENTATION in one .c file before including it.
// the caller brings the bus: write and read are whole i2c transfers to addr
// returning 0, or nonzero on a nack, delay_us waits
//
//   seesaw ss = {.io = {0, bus_write, bus_read, wait_us}, .addr = 0x2E};
//   seesaw_reset(&ss);
//   seesaw_pins_input_pullup(&ss, 1U << 7);
//   uint32_t pins;
//   seesaw_pins_read(&ss, &pins);
//
// registers from adafruit's seesaw library (bsd)
#ifndef SEESAW_H
#define SEESAW_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  void *ctx;
  int (*write)(void *ctx, uint8_t addr, const uint8_t *buf, size_t len);
  int (*read)(void *ctx, uint8_t addr, uint8_t *buf, size_t len);
  void (*delay_us)(void *ctx, uint32_t us);
} seesaw_io;

typedef struct {
  seesaw_io io;
  uint8_t addr;
} seesaw;

// every call returns 0, or nonzero once the chip stops answering

// soft reset, then checks the chip id
int seesaw_reset(seesaw *ss);
int seesaw_pins_output(seesaw *ss, uint32_t pins);
int seesaw_pins_input_pullup(seesaw *ss, uint32_t pins);
int seesaw_pins_write(seesaw *ss, uint32_t pins, int high);
int seesaw_pins_read(seesaw *ss, uint32_t *pins);
// 16 bit duty on one of the chip's pwm outputs
int seesaw_pwm(seesaw *ss, uint8_t pwm, uint16_t duty);

// raw register access: a module base and a register in it. writes take at
// most 8 bytes, a longer one returns -1 without touching the bus
int seesaw_write(seesaw *ss, uint8_t base, uint8_t reg, const uint8_t *buf,
                 size_t len);
int seesaw_read(seesaw *ss, uint8_t base, uint8_t reg, uint8_t *buf,
                size_t len);

#endif

#ifdef SEESAW_IMPLEMENTATION
#ifndef SEESAW_IMPLEMENTED
#define SEESAW_IMPLEMENTED

enum {
  SEESAW_STATUS = 0x00,
  SEESAW_STATUS_HW_ID = 0x01,
  SEESAW_STATUS_SWRST = 0x7F,
  SEESAW_GPIO = 0x01,
  SEESAW_GPIO_DIRSET = 0x02,
  SEESAW_GPIO_DIRCLR = 0x03,
  SEESAW_GPIO_BULK = 0x04,
  SEESAW_GPIO_SET = 0x05,
  SEESAW_GPIO_CLR = 0x06,
  SEESAW_GPIO_PULLENSET = 0x0B,
  SEESAW_TIMER = 0x08,
  SEESAW_TIMER_PWM = 0x01,
  // samd09 and attiny817 chips
  SEESAW_ID_SAMD09 = 0x55,
  SEESAW_ID_TINY817 = 0x87,
};

// the chip needs a moment between a register address and the read of it
#define SEESAW_READ_US 250U
#define SEESAW_MAX_WRITE 8U

int seesaw_write(seesaw *ss, uint8_t base, uint8_t reg, const uint8_t *buf,
                 size_t len) {
  uint8_t msg[2 + SEESAW_MAX_WRITE] = {base, reg};
  if (len > SEESAW_MAX_WRITE)
    return -1;
  for (size_t i = 0; i < len; i++)
    msg[2 + i] = buf[i];
  return ss->io.write(ss->io.ctx, ss->addr, msg, 2 + len);
}

int seesaw_read(seesaw *ss, uint8_t base, uint8_t reg, uint8_t *buf,
                size_t len) {
  uint8_t msg[2] = {base, reg};
  if (ss->io.write(ss->io.ctx, ss->addr, msg, 2))
    return -1;
  ss->io.delay_us(ss->io.ctx, SEESAW_READ_US);
  return ss->io.read(ss->io.ctx, ss->addr, buf, len);
}

// the chip can still be booting at power up, and it reboots after the reset,
// so both steps retry for half a second
int seesaw_reset(seesaw *ss) {
  uint8_t magic = 0xFF, id = 0;
  int tries = 50;
  while (seesaw_write(ss, SEESAW_STATUS, SEESAW_STATUS_SWRST, &magic, 1)) {
    if (!--tries)
      return -1;
    ss->io.delay_us(ss->io.ctx, 10000U);
  }
  for (tries = 50; tries--;) {
    ss->io.delay_us(ss->io.ctx, 10000U);
    if (!seesaw_read(ss, SEESAW_STATUS, SEESAW_STATUS_HW_ID, &id, 1) &&
        (id == SEESAW_ID_SAMD09 || id == SEESAW_ID_TINY817))
      return 0;
  }
  return -1;
}

// pin masks go out big endian
static int seesaw_pins(seesaw *ss, uint8_t reg, uint32_t pins) {
  uint8_t be[4] = {(uint8_t)(pins >> 24), (uint8_t)(pins >> 16),
                   (uint8_t)(pins >> 8), (uint8_t)pins};
  return seesaw_write(ss, SEESAW_GPIO, reg, be, 4);
}

int seesaw_pins_output(seesaw *ss, uint32_t pins) {
  return seesaw_pins(ss, SEESAW_GPIO_DIRSET, pins);
}

int seesaw_pins_input_pullup(seesaw *ss, uint32_t pins) {
  // the pull direction follows the output latch, so set it high for pull up
  return seesaw_pins(ss, SEESAW_GPIO_DIRCLR, pins) ||
         seesaw_pins(ss, SEESAW_GPIO_PULLENSET, pins) ||
         seesaw_pins(ss, SEESAW_GPIO_SET, pins);
}

int seesaw_pins_write(seesaw *ss, uint32_t pins, int high) {
  return seesaw_pins(ss, high ? SEESAW_GPIO_SET : SEESAW_GPIO_CLR, pins);
}

int seesaw_pins_read(seesaw *ss, uint32_t *pins) {
  uint8_t be[4];
  if (seesaw_read(ss, SEESAW_GPIO, SEESAW_GPIO_BULK, be, 4))
    return -1;
  *pins = (uint32_t)be[0] << 24 | (uint32_t)be[1] << 16 | (uint32_t)be[2] << 8 |
          be[3];
  return 0;
}

int seesaw_pwm(seesaw *ss, uint8_t pwm, uint16_t duty) {
  uint8_t msg[3] = {pwm, (uint8_t)(duty >> 8), (uint8_t)duty};
  return seesaw_write(ss, SEESAW_TIMER, SEESAW_TIMER_PWM, msg, 3);
}

#endif
#endif
