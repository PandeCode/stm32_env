// adafruit's 1.8" tft shield v2: a square you steer with the joystick, the
// joystick press changes its color, A/B/C set the backlight.
// NOTE: the shield takes spi from the 6 pin icsp header, which the nucleo
// doesn't have, so D13, D11 and D12 need wires to the shield's icsp pins
#define ST7735_IMPLEMENTATION
#include "st7735.h"
#define SEESAW_IMPLEMENTATION
#include "seesaw.h"

#include "board.h"
#include "i2c1.h"
#include "spi1.h"

// arduino D8, D10 and D4 on the nucleo
#define TFT_DC GPIOC, 7
#define TFT_CS GPIOC, 9
#define SD_CS GPIOB, 5

#define SHIELD_ADDR 0x2E
#define SHIELD_TFT_RESET (1U << 3)
#define SHIELD_UP (1U << 5)
#define SHIELD_LEFT (1U << 6)
#define SHIELD_PRESS (1U << 7)
#define SHIELD_DOWN (1U << 8)
#define SHIELD_RIGHT (1U << 9)
#define SHIELD_A (1U << 10)
#define SHIELD_B (1U << 11)
#define SHIELD_C (1U << 14)
#define SHIELD_BUTTONS                                                         \
  (SHIELD_UP | SHIELD_LEFT | SHIELD_PRESS | SHIELD_DOWN | SHIELD_RIGHT |       \
   SHIELD_A | SHIELD_B | SHIELD_C)
// the backlight hangs off the chip's first pwm output
#define SHIELD_BACKLIGHT 0

#define SIZE 16

static void tft_write(void *ctx, int data, const uint8_t *buf, size_t len) {
  (void)ctx;
  gpio_write(TFT_DC, data);
  gpio_write(TFT_CS, 0);
  spi1_write(buf, len);
  gpio_write(TFT_CS, 1);
}

static void wait_ms(void *ctx, uint32_t ms) {
  (void)ctx;
  delay_ms(ms);
}

static int shield_write(void *ctx, uint8_t addr, const uint8_t *buf,
                        size_t len) {
  (void)ctx;
  return i2c1_write(addr, buf, len);
}

static int shield_read(void *ctx, uint8_t addr, uint8_t *buf, size_t len) {
  (void)ctx;
  return i2c1_read(addr, buf, len);
}

static void wait_us(void *ctx, uint32_t us) {
  (void)ctx;
  delay_us(us);
}

static void pins_init(void) {
  gpio_output(TFT_DC, 0);
  gpio_output(TFT_CS, 1);
  // the sd card shares the bus, keep it off it
  gpio_output(SD_CS, 1);
}

static void shield_init(seesaw *ss) {
  if (seesaw_reset(ss) || seesaw_pwm(ss, SHIELD_BACKLIGHT, 0) ||
      seesaw_pins_output(ss, SHIELD_TFT_RESET) ||
      seesaw_pins_input_pullup(ss, SHIELD_BUTTONS))
    fail();
  // the tft wants a low pulse on reset before its init
  if (seesaw_pins_write(ss, SHIELD_TFT_RESET, 0))
    fail();
  delay_ms(10);
  if (seesaw_pins_write(ss, SHIELD_TFT_RESET, 1))
    fail();
  delay_ms(120);
}

int main(void) {
  pins_init();
  spi1_init();
  i2c1_init();

  seesaw ss = {.io = {0, shield_write, shield_read, wait_us},
               .addr = SHIELD_ADDR};
  shield_init(&ss);

  st7735 tft = {.io = {0, tft_write, wait_ms}};
  st7735_init(&tft);
  st7735_rotate(&tft, 1);

  const uint16_t background = st7735_rgb(0, 0, 0);
  const uint16_t colors[] = {
      st7735_rgb(255, 0, 0),   st7735_rgb(0, 255, 0),   st7735_rgb(0, 0, 255),
      st7735_rgb(255, 255, 0), st7735_rgb(255, 0, 255), st7735_rgb(0, 255, 255),
  };
  st7735_fill_rect(&tft, 0, 0, tft.width, tft.height, background);
  if (seesaw_pwm(&ss, SHIELD_BACKLIGHT, 0xFFFF))
    fail();

  int x = (tft.width - SIZE) / 2, y = (tft.height - SIZE) / 2;
  unsigned color = 0;
  uint32_t last = SHIELD_BUTTONS;
  st7735_fill_rect(&tft, x, y, SIZE, SIZE, colors[color]);

  for (;;) {
    uint32_t pins;
    if (seesaw_pins_read(&ss, &pins))
      fail();
    // buttons pull low when pressed
    uint32_t held = ~pins & SHIELD_BUTTONS;
    uint32_t pressed = held & last;
    last = pins;

    int nx = x, ny = y;
    if (held & SHIELD_UP)
      ny -= 2;
    if (held & SHIELD_DOWN)
      ny += 2;
    if (held & SHIELD_LEFT)
      nx -= 2;
    if (held & SHIELD_RIGHT)
      nx += 2;
    nx = nx < 0 ? 0 : nx > tft.width - SIZE ? tft.width - SIZE : nx;
    ny = ny < 0 ? 0 : ny > tft.height - SIZE ? tft.height - SIZE : ny;

    if (pressed & SHIELD_PRESS)
      color = (color + 1U) % (sizeof colors / sizeof colors[0]);
    if (nx != x || ny != y || pressed & SHIELD_PRESS) {
      st7735_fill_rect(&tft, x, y, SIZE, SIZE, background);
      x = nx;
      y = ny;
      st7735_fill_rect(&tft, x, y, SIZE, SIZE, colors[color]);
    }

    if ((pressed & SHIELD_A && seesaw_pwm(&ss, SHIELD_BACKLIGHT, 0xFFFF)) ||
        (pressed & SHIELD_B && seesaw_pwm(&ss, SHIELD_BACKLIGHT, 0x4000)) ||
        (pressed & SHIELD_C && seesaw_pwm(&ss, SHIELD_BACKLIGHT, 0)))
      fail();
    delay_ms(20);
  }
}
