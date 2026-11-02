# notes

Refs for stuff all from ST's pages for the [STM32H533RE](https://www.st.com/en/microcontrollers-microprocessors/stm32h533re.html) and the [NUCLEO-H533RE](https://www.st.com/en/evaluation-tools/nucleo-h533re.html):

- RM: RM0481 Rev 5, the reference manual, the big one
- DS: DS14539 Rev 3, the datasheet (pins, electrical stuff)
- UM: UM3121 Rev 6, the Nucleo board manual
- PM: PM0264 Rev 8, the Cortex-M33 programming manual
- ES0621, the errata

Page numbers are for those revisions, other copies floating around differ.

## Memory

- memory map and SRAM: RM 2.3.2 p131, table 6 p139, 2.3.3 p147

## Clocks

- HSI, what runs after reset: RM 11.4.2 p487, CR 11.8.1 p511
- turning peripherals on: AHB2ENR 11.8.27 p551, APB1LENR 11.8.29 p555,
  APB2ENR 11.8.31 p559
- kernel clocks: RM 11.4.12 p498, CCIPR3 11.8.42 p583, CCIPR5 11.8.44 p588

## Core

- SysTick: PM 4.4 p205
- CPACR (fpu on): PM 4.3.1 p183
- Thumb-2: PM 3.1 p48
- startup: `firmware.mk` links crti/crtn because ST's startup calls
  `__libc_init_array`

## GPIO

- RM 13.3.2 p618, registers 13.4.1 to 13.4.10 p626 to p631
- LD2 on PA5: UM 12.1 p22
- Arduino header pins: UM 13.1, table 16 p27
- alternate functions: DS table 15 p82 and p83, or quicker,
  [stm32h533retx-pinctrl.dtsi](https://github.com/zephyrproject-rtos/hal_stm32/blob/main/dts/st/h5/stm32h533retx-pinctrl.dtsi)

## USART

- the ST-LINK's virtual com port is USART2 on PA2/PA3: UM 12.4, table 13 p23
- baud rate RM 56.5.8 p2614
- registers: CR1 56.8.1 p2648, BRR 56.8.6 p2666, ISR 56.8.10 p2670,
  ICR 56.8.12 p2681, RDR 56.8.13 p2682, TDR 56.8.14 p2683
- printf over it: the [newlib libc manual](https://sourceware.org/newlib/libc.html),
  Syscalls

## ADC

- channels RM 28.4.2 p1199, regulator 28.4.6 p1207, calibration 28.4.8
  p1208, enable 28.4.9 p1211, sampling time 28.4.12 p1214
- temperature sensor RM 28.4.31 p1271, VREFINT 28.4.33 p1274
- the factory values: VREFINT_CAL DS table 26 p105, TS_CAL1/TS_CAL2 table 96
  p181, regulator startup p171

## SPI

- simplex RM 58.4.4 p2747, slave select 58.4.7 p2754, enable 58.4.11
  p2761, transfers and EOT 58.4.12 p2762
- registers: CR1 58.11.1 p2799, CFG1 58.11.3 p2801, CFG2 58.11.4 p2805,
  SR 58.11.6 p2808, TXDR 58.11.8 p2812
- ES0621 2.17.2: turn SPE off right after EOT

## I2C

- timings RM 53.4.5 p2312, controller mode 53.4.9 p2328
- registers: CR2 53.9.2 p2360, TIMINGR 53.9.5 p2363
- the TIMINGR values: RM 53.4.10 table 585 p2340, the 8 MHz 100 kHz column

## TFT shield

- the panel init and rotation: `initR()` and `setRotation()` in
  [Adafruit_ST7735.cpp](https://github.com/adafruit/Adafruit-ST7735-Library/blob/master/Adafruit_ST7735.cpp)
- seesaw: [Adafruit_seesaw.h](https://github.com/adafruit/Adafruit_Seesaw/blob/master/Adafruit_seesaw.h)
  and [Adafruit_TFTShield18.h](https://github.com/adafruit/Adafruit_Seesaw/blob/master/Adafruit_TFTShield18.h)
- `seesaw_shield18_test` in
  [the ST7735 examples](https://github.com/adafruit/Adafruit-ST7735-Library/tree/master/examples)
- the ICSP SPI pins:
  [Adafruit-1.8-TFT-Shield-PCB](https://github.com/adafruit/Adafruit-1.8-TFT-Shield-PCB)

## Tools

- [pyOCD configuration](https://pyocd.io/docs/configuration.html)
