// st7735r driver for the 128x160 black tab panel on adafruit's 1.8" tft.
// #define ST7735_IMPLEMENTATION in one .c file before including it.
// no mcu code in here, the caller brings the bus: write sends bytes with the
// dc pin low (command) or high (data) and chip select low, delay_ms waits
//
//   st7735 tft = {.io = {.ctx = 0, .write = bus_write, .delay_ms = wait}};
//   st7735_init(&tft);
//   st7735_fill_rect(&tft, 0, 0, tft.width, tft.height, st7735_rgb(0, 0, 255));
//
// init sequence and rotations from adafruit's st7735 library (mit)
#ifndef ST7735_H
#define ST7735_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  void *ctx;
  void (*write)(void *ctx, int data, const uint8_t *buf, size_t len);
  void (*delay_ms)(void *ctx, uint32_t ms);
} st7735_io;

typedef struct {
  st7735_io io;
  uint16_t width, height;
} st7735;

void st7735_init(st7735 *tft);
// 0 to 3, quarter turns clockwise from portrait
void st7735_rotate(st7735 *tft, unsigned rotation);
// clipped to the screen
void st7735_fill_rect(st7735 *tft, int x, int y, int w, int h, uint16_t color);

static inline uint16_t st7735_rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)((r & 0xF8U) << 8 | (g & 0xFCU) << 3 | b >> 3);
}

#endif

#ifdef ST7735_IMPLEMENTATION
#ifndef ST7735_IMPLEMENTED
#define ST7735_IMPLEMENTED

enum {
  ST7735_SWRESET = 0x01,
  ST7735_SLPOUT = 0x11,
  ST7735_NORON = 0x13,
  ST7735_INVOFF = 0x20,
  ST7735_DISPON = 0x29,
  ST7735_CASET = 0x2A,
  ST7735_RASET = 0x2B,
  ST7735_RAMWR = 0x2C,
  ST7735_MADCTL = 0x36,
  ST7735_COLMOD = 0x3A,
  ST7735_FRMCTR1 = 0xB1,
  ST7735_FRMCTR2 = 0xB2,
  ST7735_FRMCTR3 = 0xB3,
  ST7735_INVCTR = 0xB4,
  ST7735_PWCTR1 = 0xC0,
  ST7735_PWCTR2 = 0xC1,
  ST7735_PWCTR3 = 0xC2,
  ST7735_PWCTR4 = 0xC3,
  ST7735_PWCTR5 = 0xC4,
  ST7735_VMCTR1 = 0xC5,
  ST7735_GMCTRP1 = 0xE0,
  ST7735_GMCTRN1 = 0xE1,
};

// madctl bits: row and column order, row/column swap
enum { ST7735_MY = 0x80, ST7735_MX = 0x40, ST7735_MV = 0x20 };

// each entry: command, argument count, arguments, then a wait in ms when the
// count has bit 7 set
#define ST7735_WAIT 0x80

// clang-format off
static const uint8_t st7735_init_cmds[] = {
    ST7735_SWRESET, ST7735_WAIT, 150,
    ST7735_SLPOUT, ST7735_WAIT, 255,
    ST7735_FRMCTR1, 3, 0x01, 0x2C, 0x2D,
    ST7735_FRMCTR2, 3, 0x01, 0x2C, 0x2D,
    ST7735_FRMCTR3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    ST7735_INVCTR, 1, 0x07,
    ST7735_PWCTR1, 3, 0xA2, 0x02, 0x84,
    ST7735_PWCTR2, 1, 0xC5,
    ST7735_PWCTR3, 2, 0x0A, 0x00,
    ST7735_PWCTR4, 2, 0x8A, 0x2A,
    ST7735_PWCTR5, 2, 0x8A, 0xEE,
    ST7735_VMCTR1, 1, 0x0E,
    ST7735_INVOFF, 0,
    // 16 bit color
    ST7735_COLMOD, 1, 0x05,
    ST7735_GMCTRP1, 16,
        0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
        0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16,
        0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
        0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    ST7735_NORON, ST7735_WAIT, 10,
    ST7735_DISPON, ST7735_WAIT, 100,
};
// clang-format on

static void st7735_command(st7735 *tft, uint8_t cmd, const uint8_t *args,
                           size_t len) {
  tft->io.write(tft->io.ctx, 0, &cmd, 1);
  if (len)
    tft->io.write(tft->io.ctx, 1, args, len);
}

void st7735_init(st7735 *tft) {
  const uint8_t *p = st7735_init_cmds;
  const uint8_t *end = p + sizeof st7735_init_cmds;
  while (p < end) {
    uint8_t cmd = p[0], n = p[1] & ~ST7735_WAIT, wait = p[1] & ST7735_WAIT;
    st7735_command(tft, cmd, p + 2, n);
    p += 2 + n;
    if (wait) {
      // 255 stands for 500
      tft->io.delay_ms(tft->io.ctx, *p == 255 ? 500U : *p);
      p++;
    }
  }
  st7735_rotate(tft, 0);
}

void st7735_rotate(st7735 *tft, unsigned rotation) {
  // black tab panels take rgb order, the others bgr
  static const uint8_t madctl[4] = {
      ST7735_MX | ST7735_MY,
      ST7735_MY | ST7735_MV,
      0,
      ST7735_MX | ST7735_MV,
  };
  rotation %= 4U;
  st7735_command(tft, ST7735_MADCTL, &madctl[rotation], 1);
  tft->width = rotation % 2U ? 160 : 128;
  tft->height = rotation % 2U ? 128 : 160;
}

static void st7735_window(st7735 *tft, int x, int y, int w, int h) {
  uint8_t cols[4] = {0, (uint8_t)x, 0, (uint8_t)(x + w - 1)};
  uint8_t rows[4] = {0, (uint8_t)y, 0, (uint8_t)(y + h - 1)};
  st7735_command(tft, ST7735_CASET, cols, 4);
  st7735_command(tft, ST7735_RASET, rows, 4);
  st7735_command(tft, ST7735_RAMWR, 0, 0);
}

void st7735_fill_rect(st7735 *tft, int x, int y, int w, int h, uint16_t color) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > tft->width)
    w = tft->width - x;
  if (y + h > tft->height)
    h = tft->height - y;
  if (w <= 0 || h <= 0)
    return;

  st7735_window(tft, x, y, w, h);
  // the panel wants each pixel high byte first
  uint8_t chunk[64];
  for (size_t i = 0; i < sizeof chunk; i += 2) {
    chunk[i] = (uint8_t)(color >> 8);
    chunk[i + 1] = (uint8_t)color;
  }
  size_t left = (size_t)w * (size_t)h * 2U;
  while (left) {
    size_t n = left < sizeof chunk ? left : sizeof chunk;
    tft->io.write(tft->io.ctx, 1, chunk, n);
    left -= n;
  }
}

#endif
#endif
